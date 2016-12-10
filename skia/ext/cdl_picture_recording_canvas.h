/*
 * Copyright 2016 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef SKIA_EXT_CDL_PICTURE_RECORDING_CANVAS_H_
#define SKIA_EXT_CDL_PICTURE_RECORDING_CANVAS_H_

#include "cdl_common.h"

#if CDL_ENABLED

#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkColorFilter.h"
#include "third_party/skia/include/core/SkDrawLooper.h"
#include "third_party/skia/include/core/SkImageFilter.h"
#include "third_party/skia/include/core/SkPaint.h"
#include "third_party/skia/include/core/SkPathEffect.h"
#include "cdl_no_draw_canvas.h"

class CdlPictureBuffer;

class CdlPictureRecordingCanvas final : public CdlNoDrawCanvas {
 public:
  CdlPictureRecordingCanvas(CdlPictureBuffer*, const SkRect& bounds);
  ~CdlPictureRecordingCanvas() override;

  void reset(CdlPictureBuffer*, const SkRect& bounds);

  int onSave() override;
  int onSaveLayer(const SaveLayerRec&) override;
  void onRestore() override;

  void onConcat(const SkMatrix&) override;
  void onSetMatrix(const SkMatrix&) override;
  void onTranslate(SkScalar, SkScalar) override;

  void onClipRect(const SkRect&, SkCanvas::ClipOp, ClipEdgeStyle) override;
  void onClipRRect(const SkRRect&, SkCanvas::ClipOp, ClipEdgeStyle) override;
  void onClipPath(const SkPath&, SkCanvas::ClipOp, ClipEdgeStyle) override;
  void onClipRegion(const SkRegion&, SkCanvas::ClipOp) override;

  void onDrawPaint(const CdlPaint&) override;
  void onDrawPath(const SkPath&, const CdlPaint&) override;
  void onDrawRect(const SkRect&, const CdlPaint&) override;
  void onDrawRRect(const SkRRect&, const CdlPaint&) override;
  void onDrawDRRect(const SkRRect&, const SkRRect&, const CdlPaint&) override;
  void onDrawOval(const SkRect&, const CdlPaint&) override;

  void onDrawPicture(const CdlPicture* picture,
                     const SkMatrix* matrix,
                     const CdlPaint* paint) override;

  void onDrawAnnotation(const SkRect&, const char[], SkData*) override;

  void onDrawText(const void*,
                  size_t,
                  SkScalar x,
                  SkScalar y,
                  const CdlPaint&) override;
  void onDrawPosText(const void*,
                     size_t,
                     const SkPoint[],
                     const CdlPaint&) override;
  void onDrawTextBlob(const SkTextBlob*,
                      SkScalar,
                      SkScalar,
                      const CdlPaint&) override;

  void onDrawImage(const SkImage*,
                   SkScalar,
                   SkScalar,
                   const CdlPaint*) override;
  void onDrawImageRect(const SkImage*,
                       const SkRect*,
                       const SkRect&,
                       const CdlPaint*,
                       SkCanvas::SrcRectConstraint) override;
  void onDrawPoints(SkCanvas::PointMode,
                    size_t count,
                    const SkPoint pts[],
                    const CdlPaint&) override;

 private:
  typedef CdlNoDrawCanvas INHERITED;

  CdlPictureBuffer* fDL;
  bool fComputeClips;
};

#endif  // CDL_ENABLED

#endif  // SKIA_EXT_CDL_PICTURE_RECORDING_CANVAS_H_
