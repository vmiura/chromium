// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/playback/image_hijack_canvas.h"

#include "base/optional.h"
#include "cc/playback/discardable_image_map.h"
#include "cc/tiles/image_decode_cache.h"
#include "skia/ext/cdl_paint.h"

namespace cc {
namespace {

SkIRect RoundOutRect(const SkRect& rect) {
  SkIRect result;
  rect.roundOut(&result);
  return result;
}

class ScopedDecodedImageLock {
 public:
  ScopedDecodedImageLock(ImageDecodeCache* image_decode_cache,
                         sk_sp<const SkImage> image,
                         const SkRect& src_rect,
                         const SkMatrix& matrix,
                         const CdlPaint* paint)
      : image_decode_cache_(image_decode_cache),
        draw_image_(std::move(image),
                    RoundOutRect(src_rect),
                    paint ? paint->getFilterQuality() : kNone_SkFilterQuality,
                    matrix),
        decoded_draw_image_(
            image_decode_cache_->GetDecodedImageForDraw(draw_image_)) {
    DCHECK(draw_image_.image()->isLazyGenerated());
    if (paint) {
      decoded_paint_ = *paint;
      decoded_paint_->setFilterQuality(decoded_draw_image_.filter_quality());
    }
  }

  ~ScopedDecodedImageLock() {
    image_decode_cache_->DrawWithImageFinished(draw_image_,
                                               decoded_draw_image_);
  }

  const DecodedDrawImage& decoded_image() const { return decoded_draw_image_; }
  const CdlPaint* decoded_paint() const {
    return decoded_paint_ ? &decoded_paint_.value() : nullptr;
  }

 private:
  ImageDecodeCache* image_decode_cache_;
  DrawImage draw_image_;
  DecodedDrawImage decoded_draw_image_;
  base::Optional<CdlPaint> decoded_paint_;
};

}  // namespace

ImageHijackCanvas::ImageHijackCanvas(SkCanvas* canvas,
                                     ImageDecodeCache* image_decode_cache)
    : CdlPassThroughCanvas(canvas), image_decode_cache_(image_decode_cache) {}

void ImageHijackCanvas::onDrawPicture(const CdlPicture* picture,
                                      const SkMatrix* matrix,
                                      const CdlPaint* paint) {
  // Ensure that pictures are unpacked by this canvas, instead of being
  // forwarded to the raster canvas.
  CdlCanvas::onDrawPicture(picture, matrix, paint);
}

void ImageHijackCanvas::onDrawImage(const SkImage* image,
                                    SkScalar x,
                                    SkScalar y,
                                    const CdlPaint* paint) {
  if (!image->isLazyGenerated()) {
    CdlPassThroughCanvas::onDrawImage(image, x, y, paint);
    return;
  }

  SkMatrix ctm = getTotalMatrix();

  ScopedDecodedImageLock scoped_lock(
      image_decode_cache_, sk_ref_sp(image),
      SkRect::MakeIWH(image->width(), image->height()), ctm, paint);
  const DecodedDrawImage& decoded_image = scoped_lock.decoded_image();
  if (!decoded_image.image())
    return;

  DCHECK_EQ(0, static_cast<int>(decoded_image.src_rect_offset().width()));
  DCHECK_EQ(0, static_cast<int>(decoded_image.src_rect_offset().height()));
  const CdlPaint* decoded_paint = scoped_lock.decoded_paint();

  bool need_scale = !decoded_image.is_scale_adjustment_identity();
  if (need_scale) {
    CdlPassThroughCanvas::save();
    CdlPassThroughCanvas::scale(
        1.f / (decoded_image.scale_adjustment().width()),
        1.f / (decoded_image.scale_adjustment().height()));
  }
  CdlPassThroughCanvas::onDrawImage(decoded_image.image().get(), x, y,
                                    decoded_paint);
  if (need_scale)
    CdlPassThroughCanvas::restore();
}

void ImageHijackCanvas::onDrawImageRect(
    const SkImage* image,
    const SkRect* src,
    const SkRect& dst,
    const CdlPaint* paint,
    SkCanvas::SrcRectConstraint constraint) {
  if (!image->isLazyGenerated()) {
    CdlPassThroughCanvas::onDrawImageRect(image, src, dst, paint, constraint);
    return;
  }

  SkRect src_storage;
  if (!src) {
    src_storage = SkRect::MakeIWH(image->width(), image->height());
    src = &src_storage;
  }
  SkMatrix matrix;
  matrix.setRectToRect(*src, dst, SkMatrix::kFill_ScaleToFit);
  matrix.postConcat(getTotalMatrix());

  ScopedDecodedImageLock scoped_lock(image_decode_cache_, sk_ref_sp(image),
                                     *src, matrix, paint);
  const DecodedDrawImage& decoded_image = scoped_lock.decoded_image();
  if (!decoded_image.image())
    return;

  const CdlPaint* decoded_paint = scoped_lock.decoded_paint();

  SkRect adjusted_src =
      src->makeOffset(decoded_image.src_rect_offset().width(),
                      decoded_image.src_rect_offset().height());
  if (!decoded_image.is_scale_adjustment_identity()) {
    float x_scale = decoded_image.scale_adjustment().width();
    float y_scale = decoded_image.scale_adjustment().height();
    adjusted_src = SkRect::MakeXYWH(
        adjusted_src.x() * x_scale, adjusted_src.y() * y_scale,
        adjusted_src.width() * x_scale, adjusted_src.height() * y_scale);
  }
  CdlPassThroughCanvas::onDrawImageRect(decoded_image.image().get(),
                                        &adjusted_src, dst, decoded_paint,
                                        constraint);
}


}  // namespace cc
