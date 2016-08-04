#include "cc/tiles/image_decode_proxy.h"

#include "base/trace_event/trace_event.h"
#include "cc/tiles/image_decode_service.h"

namespace cc {

ImageDecodeProxy::ImageDecodeProxy() {
  mojo_thread_.reset(new base::Thread("image_decode_proxy"));
  mojo_thread_->Start();
  mojo_thread_->task_runner()->PostTask(
      FROM_HERE,
      base::Bind(&ImageDecodeProxy::OnInitializeMojo, base::Unretained(this)));
};

ImageDecodeProxy::~ImageDecodeProxy() = default;

ImageDecodeProxy* ImageDecodeProxy::Current() {
  // Use base::Singleton thingy.
  static ImageDecodeProxy proxy;
  return &proxy;
}

void ImageDecodeProxy::OnInitializeMojo() {
  VLOG(0) << "ImageDecodeProxy::InitializeMojo()";

  image_decode_mojo_.reset(
      new ImageDecodeMojo(mojo::GetProxy(&image_decode_ptr_)));
}

void ImageDecodeProxy::OnDecodeImage(uint32_t unique_id,
                                     void* data,
                                     base::WaitableEvent* event) {
  TRACE_EVENT1("cc", "ImageDecodeProxy::OnDecodeImage", "unique_id", unique_id);
  VLOG(0) << "ImageDecodeProxy::OnDecodeImage " << unique_id << " " << data;

  // TODO(hackathon): send request via image_decode_mojo_.
  image_decode_ptr_->DecodeImage(
      unique_id, (uint64_t)data,
      base::Bind(&ImageDecodeProxy::OnDecodeImageCompleted,
                 base::Unretained(this), event));
}

void ImageDecodeProxy::OnDecodeImageCompleted(base::WaitableEvent* event) {
  TRACE_EVENT0("cc", "ImageDecodeProxy::OnDecodeImageCompleted");
  VLOG(0) << "ImageDecodeProxy::OnDecodeImageCompleted";
  event->Signal();
}

bool ImageDecodeProxy::DecodeImage(uint32_t unique_id,
                                   const SkImageInfo& info,
                                   void* data) {
  base::WaitableEvent event(base::WaitableEvent::ResetPolicy::MANUAL,
                            base::WaitableEvent::InitialState::NOT_SIGNALED);

  mojo_thread_->task_runner()->PostTask(
      FROM_HERE, base::Bind(&ImageDecodeProxy::OnDecodeImage,
                            base::Unretained(this), unique_id, data, &event));

  event.Wait();

  return true;
}

ProxyImageGenerator::ProxyImageGenerator(const SkImageInfo& info,
                                         uint32_t unique_id,
                                         ImageDecodeProxy* proxy)
    : SkImageGenerator(info), unique_id_(unique_id), proxy_(proxy) {}

ProxyImageGenerator::~ProxyImageGenerator() {
  // TODO(hackathon): Should scope this a different way probably - don't want to
  // have to send over IPC?
  cc::ImageDecodeService::Current()->UnregisterImage(unique_id_);
}

bool ProxyImageGenerator::onGetPixels(const SkImageInfo& info,
                                      void* pixels,
                                      size_t rowBytes,
                                      SkPMColor ctable[],
                                      int* ctableCount) {
  // Sanity check
  if (!getInfo().validRowBytes(rowBytes))
    return false;
  if (getInfo().height() != info.height() || getInfo().width() != info.width())
    return false;

  return proxy_->DecodeImage(unique_id_, getInfo(), pixels);
}
}
