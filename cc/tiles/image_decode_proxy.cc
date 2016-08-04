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

ImageDecodeProxy::ImageDecodeProxy(ImageDecodeService* service) : proxy_thread_("ImageDecodeProxy"), service_(service) {
  base::ThreadRestrictions::SetIOAllowed(true);
  proxy_thread_.Start();
  proxy_thread_.task_runner()->PostTask(
      FROM_HERE,
      base::Bind(&ImageDecodeProxy::OnInitializeMojo, base::Unretained(this)));
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

void ImageDecodeProxy::OnInitializeMojo() {
  base::ThreadRestrictions::SetIOAllowed(true);
  service_->Bind(mojo::GetProxy(&image_decode_ptr_));
}

void ImageDecodeProxy::OnDecodeImage(
    uint32_t unique_id,
    mojo::ScopedSharedBufferHandle* remote_handle,
    size_t size,
    CompletionEvent* event) {
  TRACE_EVENT1("cc", "ImageDecodeProxy::OnDecodeImage", "unique_id", unique_id);

  // TODO(hackathon): Pass shared memory buffer instead of raw pointer.
  image_decode_ptr_->DecodeImage(
      unique_id, std::move(*remote_handle), size,
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
  size_t size = info.getSafeSize(info.minRowBytes());

  // Allocate shared memory buffer.
  mojo::ScopedSharedBufferHandle local_handle =
      mojo::SharedBufferHandle::Create(size);
  DCHECK(local_handle.is_valid());

  mojo::ScopedSharedBufferMapping mapped = local_handle->Map(size);
  DCHECK(mapped);

  mojo::ScopedSharedBufferHandle remote_handle =
      local_handle->Clone(mojo::SharedBufferHandle::AccessMode::READ_WRITE);
  CHECK(remote_handle.is_valid())
      << "Mojo error when creating read-only buffer handle.";

  CompletionEvent event;
  proxy_thread_.task_runner()->PostTask(
      FROM_HERE,
      base::Bind(&ImageDecodeProxy::OnDecodeImage, base::Unretained(this),
                 unique_id, &remote_handle, size, &event));

  event.Wait();

  void* local_data = mapped.get();

  memcpy(data, local_data, size);

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
