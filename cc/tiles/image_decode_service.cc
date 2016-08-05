// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/tiles/image_decode_service.h"
#include "cc/tiles/image_decode_proxy.h"
#include "cc/resources/resource_format_utils.h"
#include "base/strings/stringprintf.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/threading/thread_restrictions.h"

namespace cc {
namespace {
class ImageDecodeThread : public base::SimpleThread {
 public:
  ImageDecodeThread(const std::string& prefix,
                    const Options& options,
                    ImageDecodeService* service)
      : SimpleThread(prefix, options), s_service(service) {}

  void Run() final { s_service->ProcessRequestQueue(); }

 private:
  ImageDecodeService* s_service;
};

const int kNumThreads = 2;

SkImageInfo CreateImageInfo(size_t width,
                            size_t height,
                            ResourceFormat format) {
  return SkImageInfo::Make(width, height,
                           ResourceFormatToClosestSkColorType(format),
                           kPremul_SkAlphaType);
}
}  // namespace

ImageDecodeService::ImageDecodeRequest::ImageDecodeRequest(
    uint32_t image_id,
    mojo::ScopedSharedBufferHandle buffer_handle,
    uint32_t size,
    const base::Closure& callback)
    : image_id(image_id),
      buffer_handle(std::move(buffer_handle)),
      size(size),
      callback(callback) {}

ImageDecodeService::ImageDecodeRequest::~ImageDecodeRequest() {}

ImageDecodeService::ImageDecodeResult::ImageDecodeResult(
    uint32_t image_id,
    bool succeeded,
    const base::Closure& callback)
    : image_id(image_id), succeeded(succeeded), callback(callback) {}

ImageDecodeService::ImageDecodeResult::~ImageDecodeResult() {}

ImageDecodeService* ImageDecodeService::s_service = nullptr;
ImageDecodeService* ImageDecodeService::Current() {
  return s_service;
}

ImageDecodeService::ImageDecodeService()
    : image_decode_binding_(this),
      requests_cv_(&lock_),
      service_thread_("ImageDecodeService") {
  TRACE_EVENT0("cc", "ImageDecodeService::ImageDecodeService()");
  // TODO(hackathon): We need to move this to Start or something and eventually
  // join the threads.
  base::ThreadRestrictions::SetIOAllowed(true);
  for (int i = 0; i < kNumThreads; ++i) {
    std::unique_ptr<base::SimpleThread> thread(new ImageDecodeThread(
        base::StringPrintf("ImageDecodeThread%d", i + 1).c_str(),
        base::SimpleThread::Options(), this));
    thread->Start();
    threads_.push_back(std::move(thread));
  }

  service_thread_.Start();

  new_results_notifier_.reset(
      new UniqueNotifier(service_thread_.task_runner().get(),
                         base::Bind(&ImageDecodeService::ProcessResultQueue,
                                    base::Unretained(this))));
  s_service = this;
}

ImageDecodeService::~ImageDecodeService() {
  {
    base::AutoLock hold(lock_);
    shutdown_ = true;
    requests_cv_.Broadcast();
  }
  for (auto& thread : threads_)
    thread->Join();

  CompletionEvent event;
  service_thread_.task_runner()->PostTask(
      FROM_HERE, base::Bind(&ImageDecodeService::DoCloseMojoBinding,
                            base::Unretained(this), base::Unretained(&event)));
  event.Wait();
}

void ImageDecodeService::Bind(mojom::ImageDecodeRequest request) {
  base::ThreadRestrictions::SetIOAllowed(true);
  image_decode_request_ = std::move(request);
  service_thread_.task_runner()->PostTask(
      FROM_HERE, base::Bind(&ImageDecodeService::DoBindOnServiceThread,
                            base::Unretained(this)));
}

void ImageDecodeService::DoBindOnServiceThread() {
  base::ThreadRestrictions::SetIOAllowed(true);
  image_decode_binding_.Bind(std::move(image_decode_request_));
}

void ImageDecodeService::DoCloseMojoBinding(CompletionEvent* event) {
  image_decode_binding_.Close();
  event->Signal();
}

void ImageDecodeService::RegisterImage(sk_sp<SkImage> image) {
  TRACE_EVENT1("cc", "ImageDecodeService::RegisterImage", "image_id",
               image->uniqueID());
  base::AutoLock hold(lock_);
  image_map_[image->uniqueID()] = std::move(image);
}

void ImageDecodeService::UnregisterImage(uint32_t image_id) {
  TRACE_EVENT1("cc", "ImageDecodeService::UnregisterImage", "image_id",
               image_id);
  base::AutoLock hold(lock_);
  image_map_.erase(image_id);
}

void ImageDecodeService::ProcessRequestQueue() {
  base::AutoLock hold(lock_);
  while (!shutdown_) {
    if (image_decode_requests_.empty()) {
      requests_cv_.Wait();
      continue;
    }

    auto request = std::move(image_decode_requests_.front());
    image_decode_requests_.pop_front();
    bool result = [this, &request]() {
      base::AutoUnlock release(lock_);

      // Map |data| into the address space and copy out its contents.
      if (!request->buffer_handle.is_valid()) {
        LOG(ERROR) << "Invalid data handle received from renderer process.";
        return false;
      }

      mojo::ScopedSharedBufferMapping mapping =
          request->buffer_handle->Map(request->size);
      if (!mapping) {
        LOG(ERROR) << "Failed to mmap region of " << request->size << " bytes.";
        return false;
      }
      return DoDecodeImage(request->image_id, mapping.get());
    }();
    image_decode_results_.emplace_back(
        new ImageDecodeResult(request->image_id, result, request->callback));
    new_results_notifier_->Schedule();
  }
}

void ImageDecodeService::ProcessResultQueue() {
  TRACE_EVENT0("cc", "ImageDecodeService::ProcessResultQueue");
  base::AutoLock hold(lock_);
  for (auto& result : image_decode_results_)
    OnImageDecoded(result->image_id, result->succeeded, result->callback);
  image_decode_results_.clear();
}

void ImageDecodeService::DecodeImage(
    uint32_t image_id,
    mojo::ScopedSharedBufferHandle buffer_handle,
    uint32_t size,
    const DecodeImageCallback& callback) {
  TRACE_EVENT1("cc", "ImageDecodeService::DecodeImage", "image_id", image_id);
  base::AutoLock hold(lock_);

  image_decode_requests_.emplace_back(new ImageDecodeRequest(
      image_id, std::move(buffer_handle), size, callback));
  requests_cv_.Broadcast();
}

bool ImageDecodeService::DoDecodeImage(uint32_t image_id, void* buffer) {
  TRACE_EVENT1("cc", "ImageDecodeService::DoDecodeImage", "image_id", image_id);
  sk_sp<SkImage> image = [this, image_id]() {
    base::AutoLock hold(lock_);
    return image_map_[image_id];
  }();

  if (!image)
    return false;

  // TODO(hackathon): Change format.
  SkImageInfo decoded_info =
      CreateImageInfo(image->width(), image->height(), RGBA_8888);

  bool result =
      image->readPixels(decoded_info, buffer, decoded_info.minRowBytes(), 0, 0,
                        SkImage::kDisallow_CachingHint);
  return result;
}

void ImageDecodeService::OnImageDecoded(uint32_t image_id,
                                        bool succeeded,
                                        const base::Closure& callback) {
  TRACE_EVENT1("cc", "ImageDecodeService::OnImageDecoded", "image_id",
               image_id);
  callback.Run();
}

}  // namespace cc
