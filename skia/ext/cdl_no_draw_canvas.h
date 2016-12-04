/*
 * Copyright 2016 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef SKIA_EXT_CDL_NO_DRAW_CANVAS_H_
#define SKIA_EXT_CDL_NO_DRAW_CANVAS_H_

#include "cdl_canvas.h"

class CdlNoDrawCanvas : public CdlCanvas {
 public:
  CdlNoDrawCanvas(int width, int height);
  ~CdlNoDrawCanvas() override;

 protected:

  // TODO(cdl): Decide if we should skip computing clips at paint time.
  void onClipRect(const SkRect&, SkCanvas::ClipOp, ClipEdgeStyle) override {}
  void onClipRRect(const SkRRect&, SkCanvas::ClipOp, ClipEdgeStyle) override {}
  void onClipPath(const SkPath&, SkCanvas::ClipOp, ClipEdgeStyle) override {}
  void onClipRegion(const SkRegion&, SkCanvas::ClipOp) override {}

  void onDiscard() override {}

  void onDrawPaint(const SkPaint&) override {}
  void onDrawPath(const SkPath&, const SkPaint&) override {}
  void onDrawRect(const SkRect&, const SkPaint&) override {}
  void onDrawRect(const SkRect&, const CdlPaint&) override {}
  void onDrawRegion(const SkRegion&, const SkPaint&) override {}
  void onDrawOval(const SkRect&, const SkPaint&) override {}
  void onDrawArc(const SkRect&,
                         SkScalar,
                         SkScalar,
                         bool,
                         const SkPaint&) override {}
  void onDrawRRect(const SkRRect&, const SkPaint&) override {}
  void onDrawDRRect(const SkRRect&, const SkRRect&, const SkPaint&) override {}

  void onDrawDrawable(SkDrawable*, const SkMatrix*) override {}

  void onDrawPicture(const CdlPicture* picture,
                             const SkMatrix* matrix,
                             const SkPaint* paint) override {}
  void onDrawAnnotation(const SkRect&, const char[], SkData*) override {}

  void onDrawText(const void*,
                          size_t,
                          SkScalar x,
                          SkScalar y,
                          const SkPaint&) override {}
  void onDrawPosText(const void*,
                             size_t,
                             const SkPoint[],
                             const SkPaint&) override {}
  void onDrawPosTextH(const void*,
                              size_t,
                              const SkScalar[],
                              SkScalar,
                              const SkPaint&) override {}
  void onDrawTextOnPath(const void*,
                                size_t,
                                const SkPath&,
                                const SkMatrix*,
                                const SkPaint&) override {}
  void onDrawTextRSXform(const void*,
                                 size_t,
                                 const SkRSXform[],
                                 const SkRect*,
                                 const SkPaint&) override {}
  void onDrawTextBlob(const SkTextBlob*,
                              SkScalar,
                              SkScalar,
                              const SkPaint&) override {}

  void onDrawBitmap(const SkBitmap&,
                            SkScalar,
                            SkScalar,
                            const SkPaint*) override {}

  void onDrawImage(const SkImage*, SkScalar, SkScalar, const SkPaint*) override {}
  void onDrawImage(const SkImage*, SkScalar, SkScalar, const CdlPaint&) override {}

  void onDrawImageRect(const SkImage*,
                               const SkRect*,
                               const SkRect&,
                               const SkPaint*,
                               SkCanvas::SrcRectConstraint) override {}
  void onDrawImageRect(const SkImage*,
                               const SkRect*,
                               const SkRect&,
                               const CdlPaint&,
                               SkCanvas::SrcRectConstraint) override {}
  void onDrawPoints(SkCanvas::PointMode,
                            size_t count,
                            const SkPoint pts[],
                            const SkPaint&) override {}

  std::unique_ptr<SkCanvas> owned_canvas_;
};

#endif  // SKIA_EXT_CDL_NO_DRAW_CANVAS_H_
