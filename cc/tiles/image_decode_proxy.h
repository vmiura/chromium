#pragma once

#include <map>

#include "third_party/skia/include/core/SkImage.h"
#include "third_party/skia/include/core/SkImageGenerator.h"
#include "base/synchronization/waitable_event.h"
#include "cc/base/cc_export.h"

namespace cc {

class CC_EXPORT ImageDecodeProxy {
 public:
  // For now, just use a singleton.
  static ImageDecodeProxy* Current();

  bool DecodeImage(uint32_t unique_id, const SkImageInfo& info, void* data);

  void OnImageDecodeCompleted(uint32_t unique_id);

 private:
  std::map<uint32_t, base::WaitableEvent*> events_;
};

class CC_EXPORT ProxyImageGenerator : public SkImageGenerator {
 public:
  ProxyImageGenerator(const SkImageInfo& info,
                      uint32_t unique_id,
                      ImageDecodeProxy* proxy);
  bool onGetPixels(const SkImageInfo& info,
                   void* pixels,
                   size_t rowBytes,
                   SkPMColor ctable[],
                   int* ctableCount) override;

 private:
  const uint32_t unique_id_;
  ImageDecodeProxy* proxy_;
};
}
