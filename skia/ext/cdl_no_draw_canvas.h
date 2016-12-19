/*
 * Copyright 2016 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef SKIA_EXT_CDL_NO_DRAW_CANVAS_H_
#define SKIA_EXT_CDL_NO_DRAW_CANVAS_H_

#include "cdl_common.h"

#if CDL_ENABLED

#include "cdl_canvas.h"

class SK_API CdlNoDrawCanvas : public CdlCanvas {
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

  void onDrawPaint(const CdlPaint&) override {}
  void onDrawPath(const SkPath&, const CdlPaint&) override {}
  void onDrawRect(const SkRect&, const CdlPaint&) override {}
  void onDrawRRect(const SkRRect&, const CdlPaint&) override {}
  void onDrawDRRect(const SkRRect&, const SkRRect&, const CdlPaint&) override {}
  void onDrawOval(const SkRect&, const CdlPaint&) override {}

  void onDrawPicture(const CdlPicture* picture,
                     const SkMatrix* matrix,
                     const CdlPaint* paint) override {}
  void onDrawAnnotation(const SkRect&, const char[], SkData*) override {}

  void onDrawText(const void*,
                  size_t,
                  SkScalar x,
                  SkScalar y,
                  const CdlPaint&) override {}
  void onDrawPosText(const void*,
                     size_t,
                     const SkPoint[],
                     const CdlPaint&) override {}

  void onDrawTextBlob(const SkTextBlob*,
                      SkScalar,
                      SkScalar,
                      const CdlPaint&) override {}

  void onDrawImage(const SkImage*,
                   SkScalar,
                   SkScalar,
                   const CdlPaint*) override {}

  void onDrawImageRect(const SkImage*,
                       const SkRect*,
                       const SkRect&,
                       const CdlPaint*,
                       SkCanvas::SrcRectConstraint) override {}

  void onDrawPoints(SkCanvas::PointMode,
                    size_t count,
                    const SkPoint pts[],
                    const CdlPaint&) override {}
};

#else  // CDL_ENABLED

#include "third_party/skia/include/utils/SkNoDrawCanvas.h"

#endif  // CDL_ENABLED

#endif  // SKIA_EXT_CDL_NO_DRAW_CANVAS_H_
