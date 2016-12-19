/*
 * Copyright 2016 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "cdl_canvas.h"

#if CDL_ENABLED

#include "base/memory/ptr_util.h"
#include "cdl_no_draw_canvas.h"
#include "cdl_paint.h"
#include "cdl_picture.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkSurface.h"
#include "third_party/skia/include/utils/SkNoDrawCanvas.h"
#include "third_party/skia/include/utils/SkNWayCanvas.h"

// TODO(cdl) LIST
// SkRecordNoopSaveLayerDrawRestores

#define RETURN_ON_NULL(ptr) \
  do {                      \
    if (nullptr == (ptr))   \
      return;               \
  } while (0)

CdlNoDrawCanvas::CdlNoDrawCanvas(int width, int height)
    : CdlCanvas(width, height) {}
CdlNoDrawCanvas::~CdlNoDrawCanvas() {}

sk_sp<CdlCanvas> CdlCanvas::Make(SkCanvas* canvas) {
  return sk_sp<CdlCanvas>(new CdlCanvas(canvas));
}

CdlCanvas::CdlCanvas(SkCanvas* canvas) : canvas_(canvas) {}

CdlCanvas::CdlCanvas(SkBaseDevice* device) {
  owned_canvas_ = base::MakeUnique<SkCanvas>(device);
  canvas_ = owned_canvas_.get();
}

CdlCanvas::CdlCanvas(const SkBitmap& bitmap) {
  owned_canvas_ = base::MakeUnique<SkCanvas>(bitmap);
  canvas_ = owned_canvas_.get();
}

CdlCanvas::CdlCanvas(const SkBitmap& bitmap, const SkSurfaceProps& props) {
  owned_canvas_ = base::MakeUnique<SkCanvas>(bitmap, props);
  canvas_ = owned_canvas_.get();
}

CdlCanvas::CdlCanvas(int width, int height)
    : owned_canvas_(new SkNoDrawCanvas(width, height)),
      canvas_(owned_canvas_.get()) {}

CdlCanvas::~CdlCanvas() {}

///////////////////////////////////////////////////////////////////////////////
// Save / Restore
int CdlCanvas::save() {
  return this->onSave();
}

int CdlCanvas::onSave() {
  return canvas_->save();
}

int CdlCanvas::saveLayer(const SkRect* bounds, const CdlPaint* paint) {
  return this->saveLayer(SaveLayerRec(bounds, paint, 0));
}

int CdlCanvas::saveLayer(const SaveLayerRec& rec) {
  return this->onSaveLayer(rec);
}

int CdlCanvas::onSaveLayer(const SaveLayerRec& rec) {
  SkPaint sk_paint;
  if (rec.fPaint)
    sk_paint = rec.fPaint->toSkPaint();

  SkCanvas::SaveLayerRec sk_rec(rec.fBounds, rec.fPaint ? &sk_paint : nullptr,
                                rec.fBackdrop, rec.fSaveLayerFlags);
  return canvas_->saveLayer(sk_rec);
}

int CdlCanvas::saveLayerAlpha(const SkRect* bounds, U8CPU alpha) {
  if (0xFF == alpha) {
    return this->saveLayer(bounds, nullptr);
  } else {
    CdlPaint tmpPaint;
    tmpPaint.setAlpha(alpha);
    return this->saveLayer(bounds, &tmpPaint);
  }
}

int CdlCanvas::saveLayerPreserveLCDTextRequests(const SkRect* bounds,
                                                const CdlPaint* paint) {
  return this->saveLayer(
      SaveLayerRec(bounds, paint, SkCanvas::kPreserveLCDText_SaveLayerFlag));
}

void CdlCanvas::restore() {
  this->onRestore();
}

void CdlCanvas::onRestore() {
  canvas_->restore();
}

int CdlCanvas::getSaveCount() const {
  return canvas_->getSaveCount();
}

void CdlCanvas::restoreToCount(int count) {
  // sanity check
  if (count < 1) {
    count = 1;
  }

  int n = this->getSaveCount() - count;
  for (int i = 0; i < n; ++i) {
    this->restore();
  }
}

///////////////////////////////////////////////////////////////////////////////
// Transform
void CdlCanvas::concat(const SkMatrix& matrix) {
  if (matrix.isIdentity()) {
    return;
  }
  this->onConcat(matrix);
}

void CdlCanvas::rotate(SkScalar degrees) {
  SkMatrix m;
  m.setRotate(degrees);
  this->concat(m);
}

void CdlCanvas::scale(SkScalar sx, SkScalar sy) {
  SkMatrix m;
  m.setScale(sx, sy);
  this->concat(m);
}

void CdlCanvas::translate(SkScalar dx, SkScalar dy) {
  if (dx || dy) {
    this->onTranslate(dx, dy);
  }
}

void CdlCanvas::resetMatrix() {
  this->onSetMatrix(SkMatrix::I());
}

void CdlCanvas::setMatrix(const SkMatrix& matrix) {
  this->onSetMatrix(matrix);
}

const SkMatrix& CdlCanvas::getTotalMatrix() const {
  return canvas_->getTotalMatrix();
}

///////////////////////////////////////////////////////////////////////////////
// Clip
SkISize CdlCanvas::getBaseLayerSize() const {
  return canvas_->getBaseLayerSize();
}

bool CdlCanvas::getClipBounds(SkRect* bounds) const {
  return canvas_->getClipBounds(bounds);
}

bool CdlCanvas::getClipDeviceBounds(SkIRect* bounds) const {
  return canvas_->getClipDeviceBounds(bounds);
}

const SkClipStack* CdlCanvas::getClipStack() const {
  return canvas_->getClipStack();
}

bool CdlCanvas::isClipEmpty() const {
  return canvas_->isClipEmpty();
}

bool CdlCanvas::isClipRect() const {
  return canvas_->isClipRect();
}

bool CdlCanvas::quickReject(const SkRect& src) const {
  return canvas_->quickReject(src);
}

void CdlCanvas::clipRect(const SkRect& rect, SkCanvas::ClipOp op, bool doAA) {
  ClipEdgeStyle edgeStyle = doAA ? kSoft_ClipEdgeStyle : kHard_ClipEdgeStyle;
  this->onClipRect(rect, op, edgeStyle);
}

void CdlCanvas::clipRRect(const SkRRect& rrect,
                          SkCanvas::ClipOp op,
                          bool doAA) {
  ClipEdgeStyle edgeStyle = doAA ? kSoft_ClipEdgeStyle : kHard_ClipEdgeStyle;
  if (rrect.isRect()) {
    this->onClipRect(rrect.getBounds(), op, edgeStyle);
  } else {
    this->onClipRRect(rrect, op, edgeStyle);
  }
}

void CdlCanvas::clipPath(const SkPath& path, SkCanvas::ClipOp op, bool doAA) {
  ClipEdgeStyle edgeStyle = doAA ? kSoft_ClipEdgeStyle : kHard_ClipEdgeStyle;

  if (!path.isInverseFillType() && canvas_->getTotalMatrix().rectStaysRect()) {
    SkRect r;
    if (path.isRect(&r)) {
      this->onClipRect(r, op, edgeStyle);
      return;
    }
    SkRRect rrect;
    if (path.isOval(&r)) {
      rrect.setOval(r);
      this->onClipRRect(rrect, op, edgeStyle);
      return;
    }
    if (path.isRRect(&rrect)) {
      this->onClipRRect(rrect, op, edgeStyle);
      return;
    }
  }

  this->onClipPath(path, op, edgeStyle);
}

void CdlCanvas::clipRegion(const SkRegion& rgn, SkCanvas::ClipOp op) {
  this->onClipRegion(rgn, op);
}

///////////////////////////////////////////////////////////////////////////////
// Draw
void CdlCanvas::drawColor(SkColor color, SkBlendMode mode) {
  CdlPaint paint;
  paint.setColor(color);
  paint.setBlendMode(mode);
  this->drawPaint(paint);
}

void CdlCanvas::drawPaint(const CdlPaint& paint) {
  this->onDrawPaint(paint);
}

void CdlCanvas::drawPoint(SkScalar x, SkScalar y, const CdlPaint& paint) {
  SkPoint pt;

  pt.set(x, y);
  this->drawPoints(SkCanvas::kPoints_PointMode, 1, &pt, paint);
}

void CdlCanvas::drawPoint(SkScalar x, SkScalar y, SkColor color) {
  SkPoint pt;
  CdlPaint paint;

  pt.set(x, y);
  paint.setColor(color);
  this->drawPoints(SkCanvas::kPoints_PointMode, 1, &pt, paint);
}

void CdlCanvas::drawPoints(SkCanvas::PointMode mode,
                           size_t count,
                           const SkPoint pts[],
                           const CdlPaint& paint) {
  this->onDrawPoints(mode, count, pts, paint);
}

void CdlCanvas::drawLine(SkScalar x0,
                         SkScalar y0,
                         SkScalar x1,
                         SkScalar y1,
                         const CdlPaint& paint) {
  SkPoint pts[2];

  pts[0].set(x0, y0);
  pts[1].set(x1, y1);
  this->drawPoints(SkCanvas::kLines_PointMode, 2, pts, paint);
}

void CdlCanvas::drawCircle(SkScalar cx,
                           SkScalar cy,
                           SkScalar radius,
                           const CdlPaint& paint) {
  if (radius < 0) {
    radius = 0;
  }

  SkRect r;
  r.set(cx - radius, cy - radius, cx + radius, cy + radius);
  this->drawOval(r, paint);
}

void CdlCanvas::drawOval(const SkRect& r, const CdlPaint& paint) {
  this->onDrawOval(r, paint);
}

void CdlCanvas::drawRect(const SkRect& r, const CdlPaint& paint) {
  onDrawRect(r, paint);
}

void CdlCanvas::drawRoundRect(const SkRect& r,
                              SkScalar rx,
                              SkScalar ry,
                              const CdlPaint& paint) {
  if (rx > 0 && ry > 0) {
    SkRRect rrect;
    rrect.setRectXY(r, rx, ry);
    this->drawRRect(rrect, paint);
  } else {
    this->drawRect(r, paint);
  }
}

void CdlCanvas::drawRectCoords(SkScalar left,
                               SkScalar top,
                               SkScalar right,
                               SkScalar bottom,
                               const CdlPaint& paint) {
  SkRect r;
  r.set(left, top, right, bottom);
  this->drawRect(r, paint);
}

void CdlCanvas::drawRRect(const SkRRect& rrect, const CdlPaint& paint) {
  this->onDrawRRect(rrect, paint);
}

void CdlCanvas::drawDRRect(const SkRRect& outer,
                           const SkRRect& inner,
                           const CdlPaint& paint) {
  if (outer.isEmpty()) {
    return;
  }
  if (inner.isEmpty()) {
    this->drawRRect(outer, paint);
    return;
  }

  this->onDrawDRRect(outer, inner, paint);
}

void CdlCanvas::drawPath(const SkPath& path, const CdlPaint& paint) {
  this->onDrawPath(path, paint);
}

void CdlCanvas::drawBitmap(const SkBitmap& bitmap,
                           SkScalar dx,
                           SkScalar dy,
                           const CdlPaint* paint) {
  if (bitmap.drawsNothing()) {
    return;
  }
  this->onDrawImage(SkImage::MakeFromBitmap(bitmap).get(), dx, dy, paint);
}

void CdlCanvas::drawImage(const SkImage* image,
                          SkScalar x,
                          SkScalar y,
                          const CdlPaint* paint) {
  RETURN_ON_NULL(image);
  this->onDrawImage(image, x, y, paint);
}

void CdlCanvas::drawImageRect(const SkImage* image,
                              const SkRect& src,
                              const SkRect& dst,
                              const CdlPaint* paint,
                              SkCanvas::SrcRectConstraint constraint) {
  RETURN_ON_NULL(image);
  if (dst.isEmpty() || src.isEmpty()) {
    return;
  }
  this->onDrawImageRect(image, &src, dst, paint, constraint);
}

void CdlCanvas::drawImageRect(const SkImage* image,
                              const SkRect& dst,
                              const CdlPaint* paint,
                              SkCanvas::SrcRectConstraint constraint) {
  RETURN_ON_NULL(image);
  this->drawImageRect(image, SkRect::MakeIWH(image->width(), image->height()),
                      dst, paint, constraint);
}

void CdlCanvas::drawText(const void* text,
                         size_t byteLength,
                         SkScalar x,
                         SkScalar y,
                         const CdlPaint& paint) {
  this->onDrawText(text, byteLength, x, y, paint);
}

void CdlCanvas::drawPosText(const void* text,
                            size_t byteLength,
                            const SkPoint pos[],
                            const CdlPaint& paint) {
  if (byteLength) {
    this->onDrawPosText(text, byteLength, pos, paint);
  }
}

void CdlCanvas::drawTextBlob(const SkTextBlob* blob,
                             SkScalar x,
                             SkScalar y,
                             const CdlPaint& paint) {
  RETURN_ON_NULL(blob);
  this->onDrawTextBlob(blob, x, y, paint);
}

///////////////////////////////////////////////////////////////////////////////
// Misc
void CdlCanvas::flush() {
  canvas_->flush();
}

bool CdlCanvas::readPixels(SkBitmap* bitmap, int srcX, int srcY) {
  return canvas_->readPixels(bitmap, srcX, srcY);
}

bool CdlCanvas::writePixels(const SkImageInfo& origInfo,
                            const void* pixels,
                            size_t rowBytes,
                            int x,
                            int y) {
  return canvas_->writePixels(origInfo, pixels, rowBytes, x, y);
}

bool CdlCanvas::writePixels(const SkBitmap& bitmap, int x, int y) {
  return canvas_->writePixels(bitmap, x, y);
}

///////////////////////////////////////////////////////////////////////////////
// Default pass-through implementation
void CdlCanvas::onConcat(SkMatrix const& matrix) {
  canvas_->concat(matrix);
}

void CdlCanvas::onSetMatrix(SkMatrix const& matrix) {
  canvas_->setMatrix(matrix);
}

void CdlCanvas::onTranslate(float dx, float dy) {
  canvas_->translate(dx, dy);
}

void CdlCanvas::onClipRect(SkRect const& r,
                           SkRegion::Op op,
                           CdlCanvas::ClipEdgeStyle style) {
  canvas_->clipRect(r, op, style == kSoft_ClipEdgeStyle);
}
void CdlCanvas::onClipRRect(SkRRect const& r,
                            SkRegion::Op op,
                            CdlCanvas::ClipEdgeStyle style) {
  canvas_->clipRRect(r, op, style == kSoft_ClipEdgeStyle);
}
void CdlCanvas::onClipPath(SkPath const& p,
                           SkRegion::Op op,
                           CdlCanvas::ClipEdgeStyle style) {
  canvas_->clipPath(p, op, style == kSoft_ClipEdgeStyle);
}
void CdlCanvas::onClipRegion(SkRegion const& r, SkRegion::Op op) {
  canvas_->clipRegion(r, op);
}
void CdlCanvas::onDiscard() {
  canvas_->discard();
}
void CdlCanvas::onDrawPaint(CdlPaint const& paint) {
  canvas_->drawPaint(paint.toSkPaint());
}
void CdlCanvas::onDrawPath(SkPath const& p, CdlPaint const& paint) {
  canvas_->drawPath(p, paint.toSkPaint());
}
void CdlCanvas::onDrawRect(SkRect const& r, CdlPaint const& paint) {
  canvas_->drawRect(r, paint.toSkPaint());
}
void CdlCanvas::onDrawOval(SkRect const& r, CdlPaint const& paint) {
  canvas_->drawOval(r, paint.toSkPaint());
}

void CdlCanvas::onDrawRRect(SkRRect const& r, CdlPaint const& paint) {
  canvas_->drawRRect(r, paint.toSkPaint());
}
void CdlCanvas::onDrawDRRect(const SkRRect& outer,
                             const SkRRect& inner,
                             const CdlPaint& paint) {
  canvas_->drawDRRect(outer, inner, paint.toSkPaint());
}

void CdlCanvas::drawPicture(const CdlPicture* picture,
                            const SkMatrix* matrix,
                            const CdlPaint* paint) {
  RETURN_ON_NULL(picture);

  if (matrix && matrix->isIdentity()) {
    matrix = nullptr;
  }

  this->onDrawPicture(picture, matrix, paint);
}

class CdlAutoCanvasMatrixPaint {
 public:
  CdlAutoCanvasMatrixPaint(CdlCanvas* canvas,
                           const SkMatrix* matrix,
                           const CdlPaint* paint,
                           const SkRect& bounds)
      : fCanvas(canvas), fSaveCount(canvas->getSaveCount()) {
    if (paint) {
      SkRect newBounds = bounds;
      if (matrix) {
        matrix->mapRect(&newBounds);
      }
      canvas->saveLayer(&newBounds, paint);
    } else if (matrix) {
      canvas->save();
    }

    if (matrix) {
      canvas->concat(*matrix);
    }
  }

  ~CdlAutoCanvasMatrixPaint() { fCanvas->restoreToCount(fSaveCount); }

 private:
  CdlCanvas* fCanvas;
  int fSaveCount;
};

void CdlCanvas::onDrawPicture(const CdlPicture* picture,
                              const SkMatrix* matrix,
                              const CdlPaint* paint) {
  if (!paint || paint->canComputeFastBounds()) {
    SkRect bounds = picture->cullRect();

    if (paint) {
      paint->computeFastBounds(bounds, &bounds);
    }
    if (matrix) {
      matrix->mapRect(&bounds);
    }
    if (this->quickReject(bounds)) {
      return;
    }
  }

  CdlAutoCanvasMatrixPaint acmp(this, matrix, paint, picture->cullRect());
  picture->playback(this);
}

void CdlCanvas::onDrawAnnotation(SkRect const& r, char const* c, SkData* d) {
  canvas_->drawAnnotation(r, c, d);
}

void CdlCanvas::onDrawText(const void* text,
                           size_t byteLength,
                           SkScalar x,
                           SkScalar y,
                           const CdlPaint& paint) {
  canvas_->drawText(text, byteLength, x, y, paint.toSkPaint());
}

void CdlCanvas::onDrawPosText(const void* text,
                              size_t byteLength,
                              const SkPoint pos[],
                              const CdlPaint& paint) {
  canvas_->drawPosText(text, byteLength, pos, paint.toSkPaint());
}

void CdlCanvas::onDrawTextBlob(const SkTextBlob* blob,
                               SkScalar x,
                               SkScalar y,
                               const CdlPaint& paint) {
  canvas_->drawTextBlob(blob, x, y, paint.toSkPaint());
}

void CdlCanvas::onDrawImage(const SkImage* image,
                            SkScalar x,
                            SkScalar y,
                            const CdlPaint* paint) {
  SkPaint sk_paint;
  if (paint)
    sk_paint = paint->toSkPaint();
  canvas_->drawImage(image, x, y, paint ? &sk_paint : nullptr);
}

void CdlCanvas::onDrawImageRect(const SkImage* image,
                                const SkRect* src,
                                const SkRect& dst,
                                const CdlPaint* paint,
                                SkCanvas::SrcRectConstraint constraint) {
  SkPaint sk_paint;
  if (paint)
    sk_paint = paint->toSkPaint();
  canvas_->drawImageRect(image, *src, dst, paint ? &sk_paint : nullptr,
                         constraint);
}

void CdlCanvas::onDrawPoints(SkCanvas::PointMode mode,
                             size_t count,
                             const SkPoint pts[],
                             const CdlPaint& paint) {
  canvas_->drawPoints(mode, count, pts, paint.toSkPaint());
}

CdlPassThroughCanvas::CdlPassThroughCanvas(SkCanvas* canvas)
    : CdlCanvas(canvas) {}
CdlPassThroughCanvas::~CdlPassThroughCanvas() {}

#else  // CDL_ENABLED

#include "third_party/skia/include/utils/SkNWayCanvas.h"

CdlPassThroughCanvas::CdlPassThroughCanvas(SkCanvas* canvas)
    : SkNWayCanvas(canvas->getBaseLayerSize().width(),
                   canvas->getBaseLayerSize().height()) {
  SkIRect raster_bounds;
  canvas->getClipDeviceBounds(&raster_bounds);
  this->clipRect(SkRect::MakeFromIRect(raster_bounds));
  this->setMatrix(canvas->getTotalMatrix());
  this->addCanvas(canvas);
}

CdlPassThroughCanvas::~CdlPassThroughCanvas() {}

#endif  // CDL_ENABLED
