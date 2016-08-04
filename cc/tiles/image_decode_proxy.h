#pragma once

#include <map>

#include "third_party/skia/include/core/SkImage.h"
#include "third_party/skia/include/core/SkImageGenerator.h"
#include "base/synchronization/waitable_event.h"
#include "base/threading/thread.h"
#include "cc/base/cc_export.h"
#include "cc/tiles/image_decode_mojo.h"

namespace cc {

class CC_EXPORT ImageDecodeProxy {
 public:
  ImageDecodeProxy();
  ~ImageDecodeProxy();

  // For now, just use a singleton.
  static ImageDecodeProxy* Current();

  bool DecodeImage(uint32_t unique_id, const SkImageInfo& info, void* data);

 private:
  void OnInitializeMojo();
  void OnDecodeImage(uint32_t unique_id,
                     void* data,
                     base::WaitableEvent* event);
  void OnDecodeImageCompleted(base::WaitableEvent* event);

  // TODO(hackathon): ImageDecodeMojo would be owned by LTH.
  // Temporarily making ImageDecodeProxy proxy own one for testing.
  std::unique_ptr<ImageDecodeMojo> image_decode_mojo_;

  cc::mojom::ImageDecodePtr image_decode_ptr_;
  std::unique_ptr<base::Thread> mojo_thread_;

  // Lock to exclusively access all the following members that are used to
  // implement the TaskRunner and TaskGraphRunner interfaces.
  base::Lock lock_;
};

class CC_EXPORT ProxyImageGenerator : public SkImageGenerator {
 public:
  ProxyImageGenerator(const SkImageInfo& info,
                      uint32_t unique_id,
                      ImageDecodeProxy* proxy);
  ~ProxyImageGenerator() override;

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
