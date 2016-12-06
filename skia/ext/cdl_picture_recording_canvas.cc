/*
 * Copyright 2016 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "cdl_picture_recording_canvas.h"

#include "base/trace_event/trace_event.h"
#include "skia/ext/cdl_picture_buffer.h"
#include "third_party/skia/include/core/SkSurface.h"
#include "third_party/skia/include/utils/SkNoDrawCanvas.h"
#include "third_party/skia/include/utils/SkNWayCanvas.h"

#define INHERITED(method, ...) this->CdlNoDrawCanvas::method(__VA_ARGS__)

CdlPictureRecordingCanvas::CdlPictureRecordingCanvas(CdlPictureBuffer* dl, const SkRect& bounds)
    : CdlNoDrawCanvas(bounds.roundOut().width(), bounds.roundOut().height()),
      fDL(dl),
      fComputeClips(true) {}

CdlPictureRecordingCanvas::~CdlPictureRecordingCanvas() {}

void CdlPictureRecordingCanvas::reset(CdlPictureBuffer* dl, const SkRect& bounds) {
  fDL = dl;
#ifdef CDL_FRIEND_OF_SKPICTURE
  canvas_->resetForNextPicture(bounds.roundOut());
#else
  owned_canvas_.reset(new SkNWayCanvas(bounds.roundOut().width(),
                                         bounds.roundOut().height()));
  canvas_ = owned_canvas_.get();
#endif
}

int CdlPictureRecordingCanvas::onSave() {
  fDL->save();
  return INHERITED::onSave();
}

int CdlPictureRecordingCanvas::onSaveLayer(const SkCanvas::SaveLayerRec& rec) {
  fDL->saveLayer(rec.fBounds, rec.fPaint, rec.fBackdrop, rec.fSaveLayerFlags);
  return INHERITED::onSaveLayer(rec);
}

void CdlPictureRecordingCanvas::onRestore() {
  fDL->restore();
  INHERITED::onRestore();
}

void CdlPictureRecordingCanvas::onConcat(const SkMatrix& matrix) {
  fDL->concat(matrix);
  INHERITED::onConcat(matrix);
}

void CdlPictureRecordingCanvas::onSetMatrix(const SkMatrix& matrix) {
  fDL->setMatrix(matrix);
  INHERITED::onSetMatrix(matrix);
}

void CdlPictureRecordingCanvas::onTranslate(SkScalar dx, SkScalar dy) {
  fDL->translate(dx, dy);
  INHERITED::onTranslate(dx, dy);
}

void CdlPictureRecordingCanvas::onClipRect(const SkRect& rect,
                                 SkCanvas::ClipOp op,
                                 ClipEdgeStyle style) {
  fDL->clipRect(rect, op, style == kSoft_ClipEdgeStyle);
  if (fComputeClips)
    CdlCanvas::onClipRect(rect, op, style);
}
void CdlPictureRecordingCanvas::onClipRRect(const SkRRect& rrect,
                                  SkCanvas::ClipOp op,
                                  ClipEdgeStyle style) {
  fDL->clipRRect(rrect, op, style == kSoft_ClipEdgeStyle);
  if (fComputeClips)
    CdlCanvas::onClipRRect(rrect, op, style);
}
void CdlPictureRecordingCanvas::onClipPath(const SkPath& path,
                                 SkCanvas::ClipOp op,
                                 ClipEdgeStyle style) {
  fDL->clipPath(path, op, style == kSoft_ClipEdgeStyle);
  if (fComputeClips)
    CdlCanvas::onClipPath(path, op, style);
}
void CdlPictureRecordingCanvas::onClipRegion(const SkRegion& region,
                                   SkCanvas::ClipOp op) {
  fDL->clipRegion(region, op);
  if (fComputeClips)
    CdlCanvas::onClipRegion(region, op);
}

void CdlPictureRecordingCanvas::onDrawPaint(const SkPaint& paint) {
  fDL->drawPaint(paint);
}
void CdlPictureRecordingCanvas::onDrawPath(const SkPath& path, const SkPaint& paint) {
  fDL->drawPath(path, paint);
}
void CdlPictureRecordingCanvas::onDrawRect(const SkRect& rect, const SkPaint& paint) {
  fDL->drawRect(rect, paint);
}
void CdlPictureRecordingCanvas::onDrawRect(const SkRect& r, const CdlPaint& paint) {
  fDL->drawRect(r, paint);
}
/*
void CdlPictureRecordingCanvas::onDrawRegion(const SkRegion& region,
                                   const SkPaint& paint) {
  fDL->drawRegion(region, paint);
}
*/
void CdlPictureRecordingCanvas::onDrawOval(const SkRect& oval, const SkPaint& paint) {
  fDL->drawOval(oval, paint);
}
void CdlPictureRecordingCanvas::onDrawArc(const SkRect& oval,
                                SkScalar startAngle,
                                SkScalar sweepAngle,
                                bool useCenter,
                                const SkPaint& paint) {
  fDL->drawArc(oval, startAngle, sweepAngle, useCenter, paint);
}
void CdlPictureRecordingCanvas::onDrawRRect(const SkRRect& rrect, const SkPaint& paint) {
  fDL->drawRRect(rrect, paint);
}
void CdlPictureRecordingCanvas::onDrawDRRect(const SkRRect& out,
                                   const SkRRect& in,
                                   const SkPaint& paint) {
  fDL->drawDRRect(out, in, paint);
}

void CdlPictureRecordingCanvas::onDrawDrawable(SkDrawable* drawable,
                                     const SkMatrix* matrix) {
  fDL->drawDrawable(drawable, matrix);
}

void CdlPictureRecordingCanvas::onDrawPicture(const CdlPicture* picture,
                                    const SkMatrix* matrix,
                                    const SkPaint* paint) {
  fDL->drawPicture(picture, matrix, paint);
}

void CdlPictureRecordingCanvas::onDrawAnnotation(const SkRect& rect,
                                       const char key[],
                                       SkData* val) {
  fDL->drawAnnotation(rect, key, val);
}

void CdlPictureRecordingCanvas::onDrawText(const void* text,
                                 size_t bytes,
                                 SkScalar x,
                                 SkScalar y,
                                 const SkPaint& paint) {
  fDL->drawText(text, bytes, x, y, paint);
}
void CdlPictureRecordingCanvas::onDrawPosText(const void* text,
                                    size_t bytes,
                                    const SkPoint pos[],
                                    const SkPaint& paint) {
  fDL->drawPosText(text, bytes, pos, paint);
}
void CdlPictureRecordingCanvas::onDrawPosTextH(const void* text,
                                     size_t bytes,
                                     const SkScalar xs[],
                                     SkScalar y,
                                     const SkPaint& paint) {
  fDL->drawPosTextH(text, bytes, xs, y, paint);
}
void CdlPictureRecordingCanvas::onDrawTextOnPath(const void* text,
                                       size_t bytes,
                                       const SkPath& path,
                                       const SkMatrix* matrix,
                                       const SkPaint& paint) {
  fDL->drawTextOnPath(text, bytes, path, matrix, paint);
}
void CdlPictureRecordingCanvas::onDrawTextRSXform(const void* text,
                                        size_t bytes,
                                        const SkRSXform xform[],
                                        const SkRect* cull,
                                        const SkPaint& paint) {
  fDL->drawTextRSXform(text, bytes, xform, cull, paint);
}
void CdlPictureRecordingCanvas::onDrawTextBlob(const SkTextBlob* blob,
                                     SkScalar x,
                                     SkScalar y,
                                     const SkPaint& paint) {
  fDL->drawTextBlob(blob, x, y, paint);
}

void CdlPictureRecordingCanvas::onDrawBitmap(const SkBitmap& bm,
                                   SkScalar x,
                                   SkScalar y,
                                   const SkPaint* paint) {
  fDL->drawImage(SkImage::MakeFromBitmap(bm), x, y, paint);
}
/*
void CdlPictureRecordingCanvas::onDrawBitmapNine(const SkBitmap& bm,
                                       const SkIRect& center,
                                       const SkRect& dst,
                                       const SkPaint* paint) {
  fDL->drawImageNine(SkImage::MakeFromBitmap(bm), center, dst, paint);
}
void CdlPictureRecordingCanvas::onDrawBitmapRect(const SkBitmap& bm,
                                       const SkRect* src,
                                       const SkRect& dst,
                                       const SkPaint* paint,
                                       SkCanvas::SrcRectConstraint constraint) {
  fDL->drawImageRect(SkImage::MakeFromBitmap(bm), src, dst, paint, constraint);
}
void CdlPictureRecordingCanvas::onDrawBitmapLattice(const SkBitmap& bm,
                                          const SkCanvas::Lattice& lattice,
                                          const SkRect& dst,
                                          const SkPaint* paint) {
  fDL->drawImageLattice(SkImage::MakeFromBitmap(bm), lattice, dst, paint);
}
*/
void CdlPictureRecordingCanvas::onDrawImage(const SkImage* img,
                                  SkScalar x,
                                  SkScalar y,
                                  const SkPaint* paint) {
  fDL->drawImage(sk_ref_sp(img), x, y, paint);
}

void CdlPictureRecordingCanvas::onDrawImage(const SkImage* img,
                                  SkScalar x,
                                  SkScalar y,
                                  const CdlPaint& paint) {
  fDL->drawImage(sk_ref_sp(img), x, y, paint);
}
/*
void CdlPictureRecordingCanvas::onDrawImageNine(const SkImage* img,
                                      const SkIRect& center,
                                      const SkRect& dst,
                                      const SkPaint* paint) {
  fDL->drawImageNine(sk_ref_sp(img), center, dst, paint);
}
*/
void CdlPictureRecordingCanvas::onDrawImageRect(const SkImage* img,
                                      const SkRect* src,
                                      const SkRect& dst,
                                      const SkPaint* paint,
                                      SkCanvas::SrcRectConstraint constraint) {
  fDL->drawImageRect(sk_ref_sp(img), src, dst, paint, constraint);
}
void CdlPictureRecordingCanvas::onDrawImageRect(const SkImage* img,
                                      const SkRect* src,
                                      const SkRect& dst,
                                      const CdlPaint& paint,
                                      SkCanvas::SrcRectConstraint constraint) {
  fDL->drawImageRect(sk_ref_sp(img), src, dst, paint, constraint);
}
/*
void CdlPictureRecordingCanvas::onDrawImageLattice(const SkImage* img,
                                         const SkCanvas::Lattice& lattice,
                                         const SkRect& dst,
                                         const SkPaint* paint) {
  fDL->drawImageLattice(sk_ref_sp(img), lattice, dst, paint);
}

void CdlPictureRecordingCanvas::onDrawPatch(const SkPoint cubics[12],
                                  const SkColor colors[4],
                                  const SkPoint texCoords[4],
                                  SkBlendMode bmode,
                                  const SkPaint& paint) {
  fDL->drawPatch(cubics, colors, texCoords, bmode, paint);
}
*/
void CdlPictureRecordingCanvas::onDrawPoints(SkCanvas::PointMode mode,
                                   size_t count,
                                   const SkPoint pts[],
                                   const SkPaint& paint) {
  fDL->drawPoints(mode, count, pts, paint);
}
/*
void CdlPictureRecordingCanvas::onDrawVertices(SkCanvas::VertexMode mode,
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
void CdlPictureRecordingCanvas::onDrawAtlas(const SkImage* atlas,
                                  const SkRSXform xforms[],
                                  const SkRect texs[],
                                  const SkColor colors[],
                                  int count,
                                  SkBlendMode bmode,
                                  const SkRect* cull,
                                  const SkPaint* paint) {
  fDL->drawAtlas(atlas, xforms, texs, colors, count, bmode, cull, paint);
}
*/

void CdlPictureRecordingCanvas::didTranslateZ(SkScalar dz) {
  fDL->translateZ(dz);
}
void CdlPictureRecordingCanvas::onDrawShadowedPicture(const SkPicture* picture,
                                            const SkMatrix* matrix,
                                            const SkPaint* paint,
                                            const SkShadowParams& params) {
  fDL->drawShadowedPicture(picture, matrix, paint, params);
}

// CdlCanvas overrides
