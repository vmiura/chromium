/*
 * Copyright 2016 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "cdl_lite_dl.h"
#include "cdl_lite_recorder.h"
#include "third_party/skia/include/core/SkSurface.h"

CdlLiteRecorder::CdlLiteRecorder(CdlLiteDL* dl, const SkRect& bounds)
    : CdlCanvas(bounds.width(), bounds.height()), fDL(dl) {}

sk_sp<SkSurface> CdlLiteRecorder::onNewSurface(const SkImageInfo&,
                                               const SkSurfaceProps&) {
  return nullptr;
}

#ifdef SK_SUPPORT_LEGACY_DRAWFILTER
SkDrawFilter* CdlLiteRecorder::setDrawFilter(SkDrawFilter* df) {
  fDL->setDrawFilter(df);
  return SkCanvas::setDrawFilter(df);
}
#endif

void CdlLiteRecorder::willSave() {
  fDL->save();
}
SkCanvas::SaveLayerStrategy CdlLiteRecorder::getSaveLayerStrategy(
    const SaveLayerRec& rec) {
  fDL->saveLayer(rec.fBounds, rec.fPaint, rec.fBackdrop, rec.fSaveLayerFlags);
  return SkCanvas::kNoLayer_SaveLayerStrategy;
}
void CdlLiteRecorder::willRestore() {
  fDL->restore();
}

void CdlLiteRecorder::didConcat(const SkMatrix& matrix) {
  fDL->concat(matrix);
}
void CdlLiteRecorder::didSetMatrix(const SkMatrix& matrix) {
  fDL->setMatrix(matrix);
}
void CdlLiteRecorder::didTranslate(SkScalar dx, SkScalar dy) {
  fDL->translate(dx, dy);
}

void CdlLiteRecorder::onClipRect(const SkRect& rect,
                                 ClipOp op,
                                 ClipEdgeStyle style) {
  fDL->clipRect(rect, op, style == kSoft_ClipEdgeStyle);
  SkCanvas::onClipRect(rect, op, style);
}
void CdlLiteRecorder::onClipRRect(const SkRRect& rrect,
                                  ClipOp op,
                                  ClipEdgeStyle style) {
  fDL->clipRRect(rrect, op, style == kSoft_ClipEdgeStyle);
  SkCanvas::onClipRRect(rrect, op, style);
}
void CdlLiteRecorder::onClipPath(const SkPath& path,
                                 ClipOp op,
                                 ClipEdgeStyle style) {
  fDL->clipPath(path, op, style == kSoft_ClipEdgeStyle);
  SkCanvas::onClipPath(path, op, style);
}
void CdlLiteRecorder::onClipRegion(const SkRegion& region, ClipOp op) {
  fDL->clipRegion(region, op);
  SkCanvas::onClipRegion(region, op);
}

void CdlLiteRecorder::onDrawPaint(const SkPaint& paint) {
  fDL->drawPaint(paint);
}
void CdlLiteRecorder::onDrawPath(const SkPath& path, const SkPaint& paint) {
  fDL->drawPath(path, paint);
}
void CdlLiteRecorder::onDrawRect(const SkRect& rect, const SkPaint& paint) {
  fDL->drawRect(rect, paint);
}
void CdlLiteRecorder::onDrawRegion(const SkRegion& region,
                                   const SkPaint& paint) {
  fDL->drawRegion(region, paint);
}
void CdlLiteRecorder::onDrawOval(const SkRect& oval, const SkPaint& paint) {
  fDL->drawOval(oval, paint);
}
void CdlLiteRecorder::onDrawArc(const SkRect& oval,
                                SkScalar startAngle,
                                SkScalar sweepAngle,
                                bool useCenter,
                                const SkPaint& paint) {
  fDL->drawArc(oval, startAngle, sweepAngle, useCenter, paint);
}
void CdlLiteRecorder::onDrawRRect(const SkRRect& rrect, const SkPaint& paint) {
  fDL->drawRRect(rrect, paint);
}
void CdlLiteRecorder::onDrawDRRect(const SkRRect& out,
                                   const SkRRect& in,
                                   const SkPaint& paint) {
  fDL->drawDRRect(out, in, paint);
}

void CdlLiteRecorder::onDrawDrawable(SkDrawable* drawable,
                                     const SkMatrix* matrix) {
  fDL->drawDrawable(drawable, matrix);
}
void CdlLiteRecorder::onDrawPicture(const SkPicture* picture,
                                    const SkMatrix* matrix,
                                    const SkPaint* paint) {
  fDL->drawPicture(picture, matrix, paint);
}
void CdlLiteRecorder::onDrawAnnotation(const SkRect& rect,
                                       const char key[],
                                       SkData* val) {
  fDL->drawAnnotation(rect, key, val);
}

void CdlLiteRecorder::onDrawText(const void* text,
                                 size_t bytes,
                                 SkScalar x,
                                 SkScalar y,
                                 const SkPaint& paint) {
  fDL->drawText(text, bytes, x, y, paint);
}
void CdlLiteRecorder::onDrawPosText(const void* text,
                                    size_t bytes,
                                    const SkPoint pos[],
                                    const SkPaint& paint) {
  fDL->drawPosText(text, bytes, pos, paint);
}
void CdlLiteRecorder::onDrawPosTextH(const void* text,
                                     size_t bytes,
                                     const SkScalar xs[],
                                     SkScalar y,
                                     const SkPaint& paint) {
  fDL->drawPosTextH(text, bytes, xs, y, paint);
}
void CdlLiteRecorder::onDrawTextOnPath(const void* text,
                                       size_t bytes,
                                       const SkPath& path,
                                       const SkMatrix* matrix,
                                       const SkPaint& paint) {
  fDL->drawTextOnPath(text, bytes, path, matrix, paint);
}
void CdlLiteRecorder::onDrawTextRSXform(const void* text,
                                        size_t bytes,
                                        const SkRSXform xform[],
                                        const SkRect* cull,
                                        const SkPaint& paint) {
  fDL->drawTextRSXform(text, bytes, xform, cull, paint);
}
void CdlLiteRecorder::onDrawTextBlob(const SkTextBlob* blob,
                                     SkScalar x,
                                     SkScalar y,
                                     const SkPaint& paint) {
  fDL->drawTextBlob(blob, x, y, paint);
}

void CdlLiteRecorder::onDrawBitmap(const SkBitmap& bm,
                                   SkScalar x,
                                   SkScalar y,
                                   const SkPaint* paint) {
  fDL->drawImage(SkImage::MakeFromBitmap(bm), x, y, paint);
}
void CdlLiteRecorder::onDrawBitmapNine(const SkBitmap& bm,
                                       const SkIRect& center,
                                       const SkRect& dst,
                                       const SkPaint* paint) {
  fDL->drawImageNine(SkImage::MakeFromBitmap(bm), center, dst, paint);
}
void CdlLiteRecorder::onDrawBitmapRect(const SkBitmap& bm,
                                       const SkRect* src,
                                       const SkRect& dst,
                                       const SkPaint* paint,
                                       SrcRectConstraint constraint) {
  fDL->drawImageRect(SkImage::MakeFromBitmap(bm), src, dst, paint, constraint);
}
void CdlLiteRecorder::onDrawBitmapLattice(const SkBitmap& bm,
                                          const SkCanvas::Lattice& lattice,
                                          const SkRect& dst,
                                          const SkPaint* paint) {
  fDL->drawImageLattice(SkImage::MakeFromBitmap(bm), lattice, dst, paint);
}

void CdlLiteRecorder::onDrawImage(const SkImage* img,
                                  SkScalar x,
                                  SkScalar y,
                                  const SkPaint* paint) {
  fDL->drawImage(sk_ref_sp(img), x, y, paint);
}
void CdlLiteRecorder::onDrawImageNine(const SkImage* img,
                                      const SkIRect& center,
                                      const SkRect& dst,
                                      const SkPaint* paint) {
  fDL->drawImageNine(sk_ref_sp(img), center, dst, paint);
}
void CdlLiteRecorder::onDrawImageRect(const SkImage* img,
                                      const SkRect* src,
                                      const SkRect& dst,
                                      const SkPaint* paint,
                                      SrcRectConstraint constraint) {
  fDL->drawImageRect(sk_ref_sp(img), src, dst, paint, constraint);
}
void CdlLiteRecorder::onDrawImageLattice(const SkImage* img,
                                         const SkCanvas::Lattice& lattice,
                                         const SkRect& dst,
                                         const SkPaint* paint) {
  fDL->drawImageLattice(sk_ref_sp(img), lattice, dst, paint);
}

void CdlLiteRecorder::onDrawPatch(const SkPoint cubics[12],
                                  const SkColor colors[4],
                                  const SkPoint texCoords[4],
                                  SkBlendMode bmode,
                                  const SkPaint& paint) {
  fDL->drawPatch(cubics, colors, texCoords, bmode, paint);
}
void CdlLiteRecorder::onDrawPoints(SkCanvas::PointMode mode,
                                   size_t count,
                                   const SkPoint pts[],
                                   const SkPaint& paint) {
  fDL->drawPoints(mode, count, pts, paint);
}
void CdlLiteRecorder::onDrawVertices(SkCanvas::VertexMode mode,
                                     int count,
                                     const SkPoint vertices[],
                                     const SkPoint texs[],
                                     const SkColor colors[],
                                     SkBlendMode bmode,
                                     const uint16_t indices[],
                                     int indexCount,
                                     const SkPaint& paint) {
  fDL->drawVertices(mode, count, vertices, texs, colors, bmode, indices,
                    indexCount, paint);
}
void CdlLiteRecorder::onDrawAtlas(const SkImage* atlas,
                                  const SkRSXform xforms[],
                                  const SkRect texs[],
                                  const SkColor colors[],
                                  int count,
                                  SkBlendMode bmode,
                                  const SkRect* cull,
                                  const SkPaint* paint) {
  fDL->drawAtlas(atlas, xforms, texs, colors, count, bmode, cull, paint);
}

void CdlLiteRecorder::didTranslateZ(SkScalar dz) {
  fDL->translateZ(dz);
}
void CdlLiteRecorder::onDrawShadowedPicture(const SkPicture* picture,
                                            const SkMatrix* matrix,
                                            const SkPaint* paint,
                                            const SkShadowParams& params) {
  fDL->drawShadowedPicture(picture, matrix, paint, params);
}

// CdlCanvas overrides

void CdlLiteRecorder::onDrawRect(const SkRect& r, const CdlPaint& paint) {
  fDL->drawRectX(r, paint);
}
