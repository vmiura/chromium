#include "cc/tiles/image_decode_proxy.h"

namespace cc {

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

  // Send request to decode rather than filling this with red.
  event.Signal();

  // Fill to red for debug.
  if (data) {
    size_t size = info.getSafeSize(info.minRowBytes());
    size_t pixel_count = size / 4;
    uint32_t* pixels = reinterpret_cast<uint32_t*>(data);
    for (size_t i = 0; i < pixel_count; i++) {
      pixels[i] = 0xFFFF0000;
    }
  }

  event.Wait();

  return true;
}

void ImageDecodeProxy::OnImageDecodeCompleted(uint32_t unique_id) {
  events_[unique_id]->Signal();
  events_.erase(unique_id);
}

ProxyImageGenerator::ProxyImageGenerator(const SkImageInfo& info,
                                         uint32_t unique_id,
                                         ImageDecodeProxy* proxy)
    : SkImageGenerator(info), unique_id_(unique_id), proxy_(proxy) {}

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
