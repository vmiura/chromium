#include "cc/tiles/image_decode_proxy.h"

#include "base/trace_event/trace_event.h"
#include "cc/tiles/image_decode_service.h"
#include "third_party/skia/include/core/SkData.h"

namespace cc {
namespace {
struct ImageSerialization {
  uint32_t width;
  uint32_t height;
  uint32_t id;
};
}

ImageDecodeProxy::ImageDecodeProxy() {
  proxy_thread_.reset(new base::Thread("ImageDecodeProxy"));
  proxy_thread_->Start();
  proxy_thread_->task_runner()->PostTask(
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
  ImageDecodeService::Current()->Bind(mojo::GetProxy(&image_decode_ptr_));
}

void ImageDecodeProxy::OnDecodeImage(uint32_t unique_id,
                                     void* data,
                                     base::WaitableEvent* event) {
  TRACE_EVENT1("cc", "ImageDecodeProxy::OnDecodeImage", "unique_id", unique_id);

  // TODO(hackathon): Pass shared memory buffer instead of raw pointer.
  image_decode_ptr_->DecodeImage(
      unique_id, (uint64_t)data,
      base::Bind(&ImageDecodeProxy::OnDecodeImageCompleted,
                 base::Unretained(this), event));
}

void ImageDecodeProxy::OnDecodeImageCompleted(base::WaitableEvent* event) {
  TRACE_EVENT0("cc", "ImageDecodeProxy::OnDecodeImageCompleted");
  event->Signal();
}

bool ImageDecodeProxy::DecodeImage(uint32_t unique_id,
                                   const SkImageInfo& info,
                                   void* data) {
  base::WaitableEvent event(base::WaitableEvent::ResetPolicy::MANUAL,
                            base::WaitableEvent::InitialState::NOT_SIGNALED);

  proxy_thread_->task_runner()->PostTask(
      FROM_HERE, base::Bind(&ImageDecodeProxy::OnDecodeImage,
                            base::Unretained(this), unique_id, data, &event));

  event.Wait();

  return true;
}


ProxyImageGenerator::ScopedBindFactory::ScopedBindFactory()
    : previous_factory_(SkGraphics::SetImageGeneratorFromEncodedFactory(
          ProxyImageGenerator::create)) {}
ProxyImageGenerator::ScopedBindFactory::~ScopedBindFactory() {
  SkGraphics::SetImageGeneratorFromEncodedFactory(previous_factory_);
}

SkImageGenerator* ProxyImageGenerator::create(SkData* data) {
  const auto* serialization =
      reinterpret_cast<const ImageSerialization*>(data->data());
  SkImageInfo info =
      SkImageInfo::MakeN32Premul(serialization->width, serialization->height);
  return new ProxyImageGenerator(info, serialization->id,
                                 ImageDecodeProxy::Current());
}

ProxyImageGenerator::ProxyImageGenerator(const SkImageInfo& info,
                                         uint32_t unique_id,
                                         ImageDecodeProxy* proxy)
    : SkImageGenerator(info), unique_id_(unique_id), proxy_(proxy) {}

ProxyImageGenerator::~ProxyImageGenerator() {
  // TODO(hackathon): Should scope this a different way probably - don't want to
  // have to send over IPC?
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

SkData* ProxyPixelSerializer::onUseImage(const SkImage* image) {
  ImageSerialization serialization;
  serialization.width = image->width();
  serialization.height = image->height();
  serialization.id = image->uniqueID();

  return SkData::MakeWithCopy(&serialization, sizeof(serialization)).release();
}

bool ProxyPixelSerializer::onUseEncodedData(const void* data, size_t len) {
  return false;
}

SkData* ProxyPixelSerializer::onEncode(const SkPixmap& pixmap) {
  CHECK(false);
  return nullptr;
}
}
