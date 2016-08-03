#include "cc/tiles/image_decode_proxy.h"

#include "cc/tiles/image_decode_service.h"

namespace cc {

ImageDecodeProxy::ImageDecodeProxy() = default;
ImageDecodeProxy::~ImageDecodeProxy() = default;

ImageDecodeProxy* ImageDecodeProxy::Current() {
  // Use base::Singleton thingy.
  static ImageDecodeProxy proxy;
  return &proxy;
}

bool ImageDecodeProxy::DecodeImage(uint32_t unique_id,
                                   const SkImageInfo& info,
                                   void* data) {
  base::WaitableEvent event(base::WaitableEvent::ResetPolicy::MANUAL,
                            base::WaitableEvent::InitialState::NOT_SIGNALED);
  events_[unique_id] = &event;

  // TODO(hackathon): send request via IPC.
  ImageDecodeService::Current()->DecodeImage(unique_id, data);

  event.Wait();

  return true;
}

void ImageDecodeProxy::OnImageDecodeCompleted(uint32_t unique_id,
                                              bool succeeded) {
  // TODO(hackathon): pipe succeeded.
  events_[unique_id]->Signal();
  events_.erase(unique_id);
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
