// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/tiles/image_decode_service.h"
#include "cc/tiles/image_decode_proxy.h"
#include "cc/resources/resource_format_utils.h"
#include "base/strings/stringprintf.h"
#include "base/threading/thread_task_runner_handle.h"

namespace cc {
namespace {
class ImageDecodeThread : public base::SimpleThread {
 public:
  ImageDecodeThread(const std::string& prefix,
                    const Options& options,
                    ImageDecodeService* service)
      : SimpleThread(prefix, options), service_(service) {}

  void Run() final {
    service_->ProcessRequestQueue();
  }

 private:
  ImageDecodeService* service_;
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

ImageDecodeService* ImageDecodeService::Current() {
  static ImageDecodeService service;
  return &service;
}

ImageDecodeService::ImageDecodeService()
    : requests_cv_(&lock_),
      task_runner_(base::ThreadTaskRunnerHandle::Get()),
      new_results_notifier_(task_runner_.get(),
                            base::Bind(&ImageDecodeService::ProcessResultQueue,
                                       base::Unretained(this))) {
  // TODO(hackathon): We need to move this to Start or something and eventually
  // join the threads.
  for (int i = 0; i < kNumThreads; ++i) {
    std::unique_ptr<base::SimpleThread> thread(new ImageDecodeThread(
        base::StringPrintf("ImageDecodeThread%d", i + 1).c_str(),
        base::SimpleThread::Options(), this));
    thread->Start();
    threads_.push_back(std::move(thread));
  }
}

ImageDecodeService::~ImageDecodeService() = default;

void ImageDecodeService::RegisterImage(sk_sp<SkImage> image) {
  base::AutoLock hold(lock_);
  image_map_[image->uniqueID()] = std::move(image);
}

void ImageDecodeService::UnregisterImage(uint32_t image_id) {
  base::AutoLock hold(lock_);
  image_map_.erase(image_id);
}

void ImageDecodeService::ProcessRequestQueue() {
  base::AutoLock hold(lock_);
  for (;;) {
    if (image_decode_requests_.empty()) {
      requests_cv_.Wait();
      continue;
    }

    auto request = image_decode_requests_.front();
    image_decode_requests_.pop_front();
    bool result = [this, &request]() {
      base::AutoUnlock release(lock_);
      return DoDecodeImage(request.first, request.second);
    }();
    image_decode_results_.emplace_back(request.first, result);
    new_results_notifier_.Schedule();
  }
}

void ImageDecodeService::ProcessResultQueue() {
  base::AutoLock hold(lock_);
  for (auto result : image_decode_results_)
    OnImageDecoded(result.first, result.second);
  image_decode_results_.clear();
}

void ImageDecodeService::DecodeImage(uint32_t image_id, void* buffer) {
  base::AutoLock hold(lock_);
  image_decode_requests_.emplace_back(image_id, buffer);
  requests_cv_.Broadcast();
}

bool ImageDecodeService::DoDecodeImage(uint32_t image_id, void* buffer) {
  sk_sp<SkImage> image = [this, image_id]() {
    base::AutoLock hold(lock_);
    DCHECK(image_map_.find(image_id) != image_map_.end());
    return image_map_[image_id];
  }();

  // TODO(hackathon): Change format.
  SkImageInfo decoded_info =
      CreateImageInfo(image->width(), image->height(), RGBA_8888);

  bool result =
      image->readPixels(decoded_info, buffer, decoded_info.minRowBytes(), 0, 0,
                        SkImage::kDisallow_CachingHint);
  return result;
}

void ImageDecodeService::OnImageDecoded(uint32_t image_id, bool succeeded) {
  // TODO(hackathon): Send IPC. Post to the origin thread?
  ImageDecodeProxy::Current()->OnImageDecodeCompleted(image_id, succeeded);
}

}  // namespace cc
