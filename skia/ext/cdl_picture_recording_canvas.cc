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

CdlPictureRecordingCanvas::CdlPictureRecordingCanvas(CdlPictureBuffer* dl,
                                                     const SkRect& bounds)
    : CdlNoDrawCanvas(bounds.roundOut().width(), bounds.roundOut().height()),
      fDL(dl),
      fComputeClips(true) {}

CdlPictureRecordingCanvas::~CdlPictureRecordingCanvas() {}

void CdlPictureRecordingCanvas::reset(CdlPictureBuffer* dl,
                                      const SkRect& bounds) {
  fDL = dl;
#ifdef CDL_FRIEND_OF_SKPICTURE
  canvas_->resetForNextPicture(bounds.roundOut());
#else
  owned_canvas_.reset(
      new SkNWayCanvas(bounds.roundOut().width(), bounds.roundOut().height()));
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

void CdlPictureRecordingCanvas::onDrawPaint(const CdlPaint& paint) {
  fDL->drawPaint(paint);
}
void CdlPictureRecordingCanvas::onDrawPath(const SkPath& path,
                                           const SkPaint& paint) {
  fDL->drawPath(path, paint);
}
void CdlPictureRecordingCanvas::onDrawRect(const SkRect& rect,
                                           const SkPaint& paint) {
  fDL->drawRect(rect, paint);
}
void CdlPictureRecordingCanvas::onDrawRect(const SkRect& r,
                                           const CdlPaint& paint) {
  fDL->drawRect(r, paint);
}

void CdlPictureRecordingCanvas::onDrawOval(const SkRect& oval,
                                           const CdlPaint& paint) {
  fDL->drawOval(oval, paint);
}

void CdlPictureRecordingCanvas::onDrawRRect(const SkRRect& rrect,
                                            const SkPaint& paint) {
  fDL->drawRRect(rrect, paint);
}
void CdlPictureRecordingCanvas::onDrawDRRect(const SkRRect& out,
                                             const SkRRect& in,
                                             const SkPaint& paint) {
  fDL->drawDRRect(out, in, paint);
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
                                              const CdlPaint& paint) {
  fDL->drawPosText(text, bytes, pos, paint);
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

void CdlPictureRecordingCanvas::onDrawImageRect(
    const SkImage* img,
    const SkRect* src,
    const SkRect& dst,
    const SkPaint* paint,
    SkCanvas::SrcRectConstraint constraint) {
  fDL->drawImageRect(sk_ref_sp(img), src, dst, paint, constraint);
}
void CdlPictureRecordingCanvas::onDrawImageRect(
    const SkImage* img,
    const SkRect* src,
    const SkRect& dst,
    const CdlPaint& paint,
    SkCanvas::SrcRectConstraint constraint) {
  fDL->drawImageRect(sk_ref_sp(img), src, dst, paint, constraint);
}

void CdlPictureRecordingCanvas::onDrawPoints(SkCanvas::PointMode mode,
                                             size_t count,
                                             const SkPoint pts[],
                                             const CdlPaint& paint) {
  fDL->drawPoints(mode, count, pts, paint);
}
