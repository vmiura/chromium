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

// static.
ImageDecodeProxy* ImageDecodeProxy::s_proxy = nullptr;
ImageDecodeProxy* ImageDecodeProxy::Current() {
  return s_proxy;
}

ImageDecodeProxy::ImageDecodeProxy(ImageDecodeService* service)
    : proxy_thread_("ImageDecodeProxy"){
          /*
          base::ThreadRestrictions::SetIOAllowed(true);
          service->Bind(mojo::GetProxy(&image_decode_ptr_));
          proxy_thread_.Start();
          proxy_thread_.task_runner()->PostTask(
              FROM_HERE,
              base::Bind(&ImageDecodeProxy::OnInitializeMojo,
          base::Unretained(this)));
          s_proxy = this;
          */
      };

ImageDecodeProxy::ImageDecodeProxy(mojom::CompositorClient* compositor_client)
    : proxy_thread_("ImageDecodeProxy") {
  base::ThreadRestrictions::SetIOAllowed(true);
  cc::mojom::ImageDecodePtr image_decode_ptr;
  compositor_client->OnImageDecodeProxyCreated(
      mojo::GetProxy(&image_decode_ptr));

  cc::mojom::ImageDecodePtrInfo ptr_info = image_decode_ptr.PassInterface();

  proxy_thread_.Start();
  proxy_thread_.task_runner()->PostTask(
      FROM_HERE, base::Bind(&ImageDecodeProxy::OnInitializeMojo,
                            base::Unretained(this), base::Passed(&ptr_info)));
  s_proxy = this;
};

ImageDecodeProxy::~ImageDecodeProxy() {
  CompletionEvent event;
  proxy_thread_.task_runner()->PostTask(
      FROM_HERE, base::Bind(&ImageDecodeProxy::DoCloseMojo,
                            base::Unretained(this), base::Unretained(&event)));
  event.Wait();
}

void ImageDecodeProxy::DoCloseMojo(CompletionEvent* event) {
  image_decode_ptr_.reset();
  event->Signal();
}

void ImageDecodeProxy::OnInitializeMojo(
    cc::mojom::ImageDecodePtrInfo ptr_info) {
  base::ThreadRestrictions::SetIOAllowed(true);
  // service_->Bind(mojo::GetProxy(&image_decode_ptr_));
  image_decode_ptr_.Bind(std::move(ptr_info));
}

void ImageDecodeProxy::OnDecodeImage(uint32_t unique_id,
                                     void* data,
                                     CompletionEvent* event) {
  TRACE_EVENT1("cc", "ImageDecodeProxy::OnDecodeImage", "unique_id", unique_id);

  // TODO(hackathon): Pass shared memory buffer instead of raw pointer.
  image_decode_ptr_->DecodeImage(
      unique_id, (uint64_t)data,
      base::Bind(&ImageDecodeProxy::OnDecodeImageCompleted,
                 base::Unretained(this), event));
}

void ImageDecodeProxy::OnDecodeImageCompleted(CompletionEvent* event) {
  TRACE_EVENT0("cc", "ImageDecodeProxy::OnDecodeImageCompleted");
  event->Signal();
}

bool ImageDecodeProxy::DecodeImage(uint32_t unique_id,
                                   const SkImageInfo& info,
                                   void* data) {
  CompletionEvent event;
  proxy_thread_.task_runner()->PostTask(
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
