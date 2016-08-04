#pragma once

#include <map>

#include "third_party/skia/include/core/SkImage.h"
#include "third_party/skia/include/core/SkImageGenerator.h"
#include "third_party/skia/include/core/SkPixelSerializer.h"
#include "third_party/skia/include/core/SkGraphics.h"
#include "base/synchronization/waitable_event.h"
#include "base/threading/thread.h"
#include "cc/base/cc_export.h"
#include "cc/ipc/image_decode.mojom.h"

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

  cc::mojom::ImageDecodePtr image_decode_ptr_;
  std::unique_ptr<base::Thread> proxy_thread_;

  // Lock to exclusively access all the following members that are used to
  // implement the TaskRunner and TaskGraphRunner interfaces.
  base::Lock lock_;
};

// A generator which acts as a proxy back to the renderer process.
class CC_EXPORT ProxyImageGenerator : public SkImageGenerator {
 public:
  // Declare this to set Skia's global ImageGenerator factory for the lifetime
  // of the object. Reverts to previous factory when it goes out of scope.
  class ScopedBindFactory {
   public:
    ScopedBindFactory();
    ~ScopedBindFactory();

   private:
    SkGraphics::ImageGeneratorFromEncodedFactory previous_factory_;
  };

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
  static SkImageGenerator* create(SkData* data);
  const uint32_t unique_id_;
  ImageDecodeProxy* proxy_;
};

// Used by Skia to serialize images for transport from Renderer > MUS process.
class CC_EXPORT ProxyPixelSerializer : public SkPixelSerializer {
 protected:
  bool onUseEncodedData(const void* data, size_t len) override;
  SkData* onUseImage(const SkImage* image) override;
  SkData* onEncode(const SkPixmap& pixmap) override;
};
}
