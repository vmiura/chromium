/*
 * Copyright 2016 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "cdl_canvas.h"
#include "cdl_paint.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkSurface.h"
#include "third_party/skia/include/utils/SkNWayCanvas.h"

CdlCanvas::CdlCanvas(SkCanvas* canvas) : canvas_(canvas) {}

CdlCanvas::CdlCanvas(int width, int height)
    : canvas_(new SkNWayCanvas(width, height)) {}

CdlCanvas::~CdlCanvas() {}

sk_sp<CdlCanvas> CdlCanvas::Make(SkCanvas* canvas) {
  return sk_sp<CdlCanvas>(new CdlCanvas(canvas));
}

int CdlCanvas::save() {
  // TODO(cdl): Deferred willSave()
  this->willSave();
  return canvas_->save();
}

void CdlCanvas::restore() {
  // TODO(cdl): Check for underflow.
  this->willRestore();
  canvas_->restore();
  this->didRestore();
}

void CdlCanvas::drawDrawable(SkDrawable* dr, SkScalar x, SkScalar y) {
  if (!dr)
    return;
  if (x || y) {
    SkMatrix matrix = SkMatrix::MakeTrans(x, y);
    this->onDrawDrawable(dr, &matrix);
  } else {
    this->onDrawDrawable(dr, nullptr);
  }
}

void CdlCanvas::drawDrawable(SkDrawable* dr, const SkMatrix* matrix) {
  if (!dr)
    return;
  if (matrix && matrix->isIdentity()) {
    matrix = nullptr;
  }
  this->onDrawDrawable(dr, matrix);
}

void CdlCanvas::flush() {
  canvas_->flush();
}

bool CdlCanvas::writePixels(const SkImageInfo& origInfo,
                            const void* pixels,
                            size_t rowBytes,
                            int x,
                            int y) {
  return canvas_->writePixels(origInfo, pixels, rowBytes, x, y);
}

void CdlCanvas::drawImage(const SkImage* image,
                          SkScalar x,
                          SkScalar y,
                          const SkPaint* paint) {
  if (!image)
    return;
  this->onDrawImage(image, x, y, paint);
}
void CdlCanvas::drawImage(const SkImage* image,
                          SkScalar x,
                          SkScalar y,
                          const CdlPaint& paint) {
  if (!image)
    return;
  this->onDrawImage(image, x, y, paint);
}

void CdlCanvas::drawImageRect(const SkImage* image,
                              const SkRect& src,
                              const SkRect& dst,
                              const SkPaint* paint,
                              SkCanvas::SrcRectConstraint constraint) {
  if (!image)
    return;
  if (dst.isEmpty() || src.isEmpty()) {
    return;
  }
  this->onDrawImageRect(image, &src, dst, paint, constraint);
}
void CdlCanvas::drawImageRect(const SkImage* image,
                              const SkRect& src,
                              const SkRect& dst,
                              const CdlPaint& paint,
                              SkCanvas::SrcRectConstraint constraint) {
  if (!image)
    return;
  if (dst.isEmpty() || src.isEmpty()) {
    return;
  }
  this->onDrawImageRect(image, &src, dst, paint, constraint);
}

void CdlCanvas::translate(SkScalar dx, SkScalar dy) {
  if (dx || dy) {
    canvas_->translate(dx, dy);
    this->didTranslate(dx, dy);
  }
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

void CdlCanvas::concat(const SkMatrix& matrix) {
  if (matrix.isIdentity()) {
    return;
  }
  canvas_->concat(matrix);
  this->didConcat(matrix);
}

void CdlCanvas::setMatrix(const SkMatrix& matrix) {
  canvas_->setMatrix(matrix);
  this->didSetMatrix(matrix);
}

void CdlCanvas::resetMatrix() {
  canvas_->resetMatrix();
  this->setMatrix(SkMatrix::I());
}

void CdlCanvas::clipRect(const SkRect& rect, SkCanvas::ClipOp op, bool doAA) {
  canvas_->clipRect(rect, op, doAA);
  ClipEdgeStyle edgeStyle = doAA ? kSoft_ClipEdgeStyle : kHard_ClipEdgeStyle;
  this->onClipRect(rect, op, edgeStyle);
}

void CdlCanvas::clipRRect(const SkRRect& rrect,
                          SkCanvas::ClipOp op,
                          bool doAA) {
  canvas_->clipRRect(rrect, op, doAA);
  ClipEdgeStyle edgeStyle = doAA ? kSoft_ClipEdgeStyle : kHard_ClipEdgeStyle;
  if (rrect.isRect()) {
    this->onClipRect(rrect.getBounds(), op, edgeStyle);
  } else {
    this->onClipRRect(rrect, op, edgeStyle);
  }
}

void CdlCanvas::clipPath(const SkPath& path, SkCanvas::ClipOp op, bool doAA) {
  canvas_->clipPath(path, op, doAA);
  ClipEdgeStyle edgeStyle = doAA ? kSoft_ClipEdgeStyle : kHard_ClipEdgeStyle;
  this->onClipPath(path, op, edgeStyle);
}

void CdlCanvas::clipRegion(const SkRegion& rgn, SkCanvas::ClipOp op) {
  canvas_->clipRegion(rgn, op);
  this->onClipRegion(rgn, op);
}

bool CdlCanvas::readPixels(SkBitmap* bitmap, int srcX, int srcY) {
  return canvas_->readPixels(bitmap, srcX, srcY);
}
void CdlCanvas::drawRect(const SkRect& r, const CdlPaint& paint) {
  onDrawRect(r, paint);
}
void CdlCanvas::drawRect(SkRect const& r, SkPaint const& paint) {
  onDrawRect(r, paint);
}

void CdlCanvas::drawPaint(const SkPaint& paint) {
  this->onDrawPaint(paint);
}

void CdlCanvas::drawColor(SkColor color, SkBlendMode mode) {
  SkPaint paint;
  paint.setColor(color);
  paint.setBlendMode(mode);
  this->drawPaint(paint);
}

void CdlCanvas::drawPoints(SkCanvas::PointMode mode,
                           size_t count,
                           const SkPoint pts[],
                           const SkPaint& paint) {
  this->onDrawPoints(mode, count, pts, paint);
}

void CdlCanvas::drawPoint(SkScalar x, SkScalar y, const SkPaint& paint) {
  SkPoint pt;

  pt.set(x, y);
  this->drawPoints(SkCanvas::kPoints_PointMode, 1, &pt, paint);
}

void CdlCanvas::drawPoint(SkScalar x, SkScalar y, SkColor color) {
  SkPoint pt;
  SkPaint paint;

  pt.set(x, y);
  paint.setColor(color);
  this->drawPoints(SkCanvas::kPoints_PointMode, 1, &pt, paint);
}

void CdlCanvas::drawLine(SkScalar x0,
                         SkScalar y0,
                         SkScalar x1,
                         SkScalar y1,
                         const SkPaint& paint) {
  SkPoint pts[2];

  pts[0].set(x0, y0);
  pts[1].set(x1, y1);
  this->drawPoints(SkCanvas::kLines_PointMode, 2, pts, paint);
}

void CdlCanvas::drawCircle(SkScalar cx,
                           SkScalar cy,
                           SkScalar radius,
                           const SkPaint& paint) {
  if (radius < 0) {
    radius = 0;
  }

  SkRect r;
  r.set(cx - radius, cy - radius, cx + radius, cy + radius);
  this->drawOval(r, paint);
}

void CdlCanvas::drawOval(const SkRect& r, const SkPaint& paint) {
  this->onDrawOval(r, paint);
}

void CdlCanvas::drawRRect(const SkRRect& rrect, const SkPaint& paint) {
  this->onDrawRRect(rrect, paint);
}

void CdlCanvas::drawDRRect(const SkRRect& outer,
                           const SkRRect& inner,
                           const SkPaint& paint) {
  if (outer.isEmpty()) {
    return;
  }
  if (inner.isEmpty()) {
    this->drawRRect(outer, paint);
    return;
  }

  this->onDrawDRRect(outer, inner, paint);
}

void CdlCanvas::drawRoundRect(const SkRect& r,
                              SkScalar rx,
                              SkScalar ry,
                              const SkPaint& paint) {
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
                               const SkPaint& paint) {
  SkRect r;
  r.set(left, top, right, bottom);
  this->drawRect(r, paint);
}

void CdlCanvas::drawPath(const SkPath& path, const SkPaint& paint) {
  this->onDrawPath(path, paint);
}

void CdlCanvas::drawBitmap(const SkBitmap& bitmap,
                           SkScalar dx,
                           SkScalar dy,
                           const SkPaint* paint) {
  if (bitmap.drawsNothing()) {
    return;
  }
  this->onDrawBitmap(bitmap, dx, dy, paint);
}

void CdlCanvas::drawPosText(const void* text,
                            size_t byteLength,
                            const SkPoint pos[],
                            const SkPaint& paint) {
  if (byteLength) {
    this->onDrawPosText(text, byteLength, pos, paint);
  }
}

void CdlCanvas::drawTextBlob(const SkTextBlob* blob,
                             SkScalar x,
                             SkScalar y,
                             const SkPaint& paint) {
  if (!blob)
    return;
  this->onDrawTextBlob(blob, x, y, paint);
}

int CdlCanvas::getSaveCount() const {
  return canvas_->getSaveCount();
}
const SkClipStack* CdlCanvas::getClipStack() const {
  return canvas_->getClipStack();
}
SkImageInfo CdlCanvas::imageInfo() const {
  return canvas_->imageInfo();
}
const SkMatrix& CdlCanvas::getTotalMatrix() const {
  return canvas_->getTotalMatrix();
}
bool CdlCanvas::quickReject(const SkRect& src) const {
  return canvas_->quickReject(src);
}

int CdlCanvas::saveLayer(const SkRect* bounds, const SkPaint* paint) {
  return this->saveLayer(SkCanvas::SaveLayerRec(bounds, paint, 0));
}

SkCanvas* CdlCanvas::getSkCanvas() {
  return canvas_;
}

CdlCanvas::SaveLayerStrategy CdlCanvas::getSaveLayerStrategy(
    const SkCanvas::SaveLayerRec&) {
  return CdlCanvas::kNoLayer_SaveLayerStrategy;
}

int CdlCanvas::saveLayer(const SkCanvas::SaveLayerRec& origRec) {
  SkCanvas::SaveLayerRec rec(origRec);
  this->getSaveLayerStrategy(rec);
  return canvas_->saveLayer(origRec);
}

int CdlCanvas::saveLayerAlpha(const SkRect* bounds, U8CPU alpha) {
  if (0xFF == alpha) {
    return this->saveLayer(bounds, nullptr);
  } else {
    SkPaint tmpPaint;
    tmpPaint.setAlpha(alpha);
    return this->saveLayer(bounds, &tmpPaint);
  }
}

int CdlCanvas::saveLayerPreserveLCDTextRequests(const SkRect* bounds,
                                                const SkPaint* paint) {
  return this->saveLayer(SkCanvas::SaveLayerRec(
      bounds, paint, SkCanvas::kPreserveLCDText_SaveLayerFlag));
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

bool CdlCanvas::getClipBounds(SkRect* bounds) const {
  return canvas_->getClipBounds(bounds);
}
bool CdlCanvas::getClipDeviceBounds(SkIRect* bounds) const {
  return canvas_->getClipDeviceBounds(bounds);
}
SkISize CdlCanvas::getBaseLayerSize() const {
  return canvas_->getBaseLayerSize();
}

bool CdlCanvas::isClipEmpty() const {
  return canvas_->isClipEmpty();
}

bool CdlCanvas::isClipRect() const {
  return canvas_->isClipRect();
}

sk_sp<SkSurface> CdlCanvas::onNewSurface(SkImageInfo const&,
                                         SkSurfaceProps const&) {
  return nullptr;
}

void CdlCanvas::didConcat(SkMatrix const&) {}
void CdlCanvas::didSetMatrix(SkMatrix const&) {}
void CdlCanvas::didTranslate(float, float) {}

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
void CdlCanvas::onDrawPaint(SkPaint const& paint) {
  canvas_->drawPaint(paint);
}
void CdlCanvas::onDrawPath(SkPath const& p, SkPaint const& paint) {
  canvas_->drawPath(p, paint);
}
void CdlCanvas::onDrawRect(SkRect const& r, SkPaint const& paint) {
  canvas_->drawRect(r, paint);
}
void CdlCanvas::onDrawRegion(SkRegion const& r, SkPaint const& paint) {
  canvas_->drawRegion(r, paint);
}
void CdlCanvas::onDrawOval(SkRect const& r, SkPaint const& paint) {
  canvas_->drawOval(r, paint);
}
void CdlCanvas::onDrawArc(const SkRect& oval,
                          SkScalar startAngle,
                          SkScalar sweepAngle,
                          bool useCenter,
                          const SkPaint& paint) {
  canvas_->drawArc(oval, startAngle, sweepAngle, useCenter, paint);
}
void CdlCanvas::onDrawRRect(SkRRect const& r, SkPaint const& paint) {
  canvas_->drawRRect(r, paint);
}
void CdlCanvas::onDrawDRRect(const SkRRect& outer,
                             const SkRRect& inner,
                             const SkPaint& paint) {
  canvas_->drawDRRect(outer, inner, paint);
}
void CdlCanvas::onDrawDrawable(SkDrawable* d, SkMatrix const* m) {
  canvas_->drawDrawable(d, m);
}
// void CdlCanvas::onDrawPicture(SkPicture const* p, SkMatrix const* m, SkPaint
// const* paint) { canvas_->drawPicture(p, m, paint); }
void CdlCanvas::onDrawAnnotation(SkRect const& r, char const* c, SkData* d) {
  canvas_->drawAnnotation(r, c, d);
}
void CdlCanvas::onDrawText(const void* text,
                           size_t byteLength,
                           SkScalar x,
                           SkScalar y,
                           const SkPaint& paint) {
  canvas_->drawText(text, byteLength, x, y, paint);
}
void CdlCanvas::onDrawPosText(const void* text,
                              size_t byteLength,
                              const SkPoint pos[],
                              const SkPaint& paint) {
  canvas_->drawPosText(text, byteLength, pos, paint);
}
void CdlCanvas::onDrawPosTextH(const void* text,
                               size_t byteLength,
                               const SkScalar xpos[],
                               SkScalar constY,
                               const SkPaint& paint) {
  canvas_->drawPosTextH(text, byteLength, xpos, constY, paint);
}
void CdlCanvas::onDrawTextOnPath(const void* text,
                                 size_t byteLength,
                                 const SkPath& path,
                                 const SkMatrix* matrix,
                                 const SkPaint& paint) {
  canvas_->drawTextOnPath(text, byteLength, path, matrix, paint);
}
void CdlCanvas::onDrawTextRSXform(const void* text,
                                  size_t byteLength,
                                  const SkRSXform xform[],
                                  const SkRect* cullRect,
                                  const SkPaint& paint) {
  canvas_->drawTextRSXform(text, byteLength, xform, cullRect, paint);
}
void CdlCanvas::onDrawTextBlob(const SkTextBlob* blob,
                               SkScalar x,
                               SkScalar y,
                               const SkPaint& paint) {
  canvas_->drawTextBlob(blob, x, y, paint);
}
void CdlCanvas::onDrawBitmap(const SkBitmap& bitmap,
                             SkScalar x,
                             SkScalar y,
                             const SkPaint* paint) {
  canvas_->drawBitmap(bitmap, x, y, paint);
}
// void CdlCanvas::onDrawBitmapLattice(SkBitmap const&, SkCanvas::Lattice
// const&, SkRect const&, SkPaint const*) {}
// void CdlCanvas::onDrawBitmapNine(SkBitmap const&, SkIRect const&, SkRect
// const&, SkPaint const*) {}
// void CdlCanvas::onDrawBitmapRect(SkBitmap const&, SkRect const*, SkRect
// const&, SkPaint const*, SkCanvas::SrcRectConstraint) {}
void CdlCanvas::onDrawImage(const SkImage* image,
                            SkScalar x,
                            SkScalar y,
                            const SkPaint* paint) {
  canvas_->drawImage(image, x, y, paint);
}
void CdlCanvas::onDrawImage(const SkImage* image,
                            SkScalar x,
                            SkScalar y,
                            const CdlPaint& paint) {
  SkPaint pt = paint.toSkPaint();
  canvas_->drawImage(image, x, y, &pt);
}
// void CdlCanvas::onDrawImageLattice(SkImage const*, SkCanvas::Lattice const&,
// SkRect const&, SkPaint const*) {}
// void CdlCanvas::onDrawImageNine(SkImage const*, SkIRect const&, SkRect
// const&, SkPaint const*) {}
void CdlCanvas::onDrawImageRect(const SkImage* image,
                                const SkRect* src,
                                const SkRect& dst,
                                const SkPaint* paint,
                                SkCanvas::SrcRectConstraint constraint) {
  canvas_->drawImageRect(image, *src, dst, paint, constraint);
}
void CdlCanvas::onDrawImageRect(const SkImage* image,
                                const SkRect* src,
                                const SkRect& dst,
                                const CdlPaint& paint,
                                SkCanvas::SrcRectConstraint constraint) {
  SkPaint pt = paint.toSkPaint();
  canvas_->drawImageRect(image, *src, dst, &pt, constraint);
}
// void CdlCanvas::onDrawPatch(SkPoint const*, unsigned int const*, SkPoint
// const*, SkBlendMode, SkPaint const&) {}
void CdlCanvas::onDrawPoints(SkCanvas::PointMode mode,
                             size_t count,
                             const SkPoint pts[],
                             const SkPaint& paint) {
  canvas_->drawPoints(mode, count, pts, paint);
}
// void CdlCanvas::onDrawVertices(SkCanvas::VertexMode, int, SkPoint const*,
// SkPoint const*, unsigned int const*, SkBlendMode, unsigned short const*, int,
// SkPaint const&) {}
// void CdlCanvas::onDrawAtlas(SkImage const*, SkRSXform const*, SkRect const*,
// unsigned int const*, int, SkBlendMode, SkRect const*, SkPaint const*) {}

// Default CdlPaint -> SkPaint implementation.
void CdlCanvas::onDrawRect(const SkRect& r, const CdlPaint& paint) {
  canvas_->drawRect(r, paint.toSkPaint());
}
