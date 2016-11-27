/*
 * Copyright 2016 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef SKIA_EXT_CDL_LITE_RECORDER_H_
#define SKIA_EXT_CDL_LITE_RECORDER_H_

#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkColorFilter.h"
#include "third_party/skia/include/core/SkDrawLooper.h"
#include "third_party/skia/include/core/SkImageFilter.h"
#include "third_party/skia/include/core/SkPaint.h"
#include "third_party/skia/include/core/SkPathEffect.h"
#include "cdl_canvas.h"

class CdlPaint;
class CdlCanvas;
class CdlLiteDL;

class CdlLiteRecorder final : public CdlCanvas {
 public:
  CdlLiteRecorder(CdlLiteDL*, const SkRect& bounds);
  ~CdlLiteRecorder() override;
  
  void reset(CdlLiteDL*, const SkRect& bounds);

  int  onSave() override;
  int  onSaveLayer(const SkCanvas::SaveLayerRec&) override;
  void onRestore() override;

  void onConcat(const SkMatrix&) override;
  void onSetMatrix(const SkMatrix&) override;
  void onTranslate(SkScalar, SkScalar) override;

  void onClipRect(const SkRect&, SkCanvas::ClipOp, ClipEdgeStyle) override;
  void onClipRRect(const SkRRect&, SkCanvas::ClipOp, ClipEdgeStyle) override;
  void onClipPath(const SkPath&, SkCanvas::ClipOp, ClipEdgeStyle) override;
  void onClipRegion(const SkRegion&, SkCanvas::ClipOp) override;

  void onDrawPaint(const SkPaint&) override;
  void onDrawPath(const SkPath&, const SkPaint&) override;
  void onDrawRect(const SkRect&, const SkPaint&) override;
  void onDrawRect(const SkRect&, const CdlPaint&) override;
  //void onDrawRegion(const SkRegion&, const SkPaint&) override;
  void onDrawOval(const SkRect&, const SkPaint&) override;
  void onDrawArc(const SkRect&,
                 SkScalar,
                 SkScalar,
                 bool,
                 const SkPaint&) override;
  void onDrawRRect(const SkRRect&, const SkPaint&) override;
  void onDrawDRRect(const SkRRect&, const SkRRect&, const SkPaint&) override;

  void onDrawDrawable(SkDrawable*, const SkMatrix*) override;
  /*
  void onDrawPicture(const SkPicture*,
                     const SkMatrix*,
                     const SkPaint*) override;
                     */
  void onDrawPicture(const CdlPicture* picture,
                     const SkMatrix* matrix,
                     const SkPaint* paint) override;

  void onDrawAnnotation(const SkRect&, const char[], SkData*) override;

  void onDrawText(const void*,
                  size_t,
                  SkScalar x,
                  SkScalar y,
                  const SkPaint&) override;
  void onDrawPosText(const void*,
                     size_t,
                     const SkPoint[],
                     const SkPaint&) override;
  void onDrawPosTextH(const void*,
                      size_t,
                      const SkScalar[],
                      SkScalar,
                      const SkPaint&) override;
  void onDrawTextOnPath(const void*,
                        size_t,
                        const SkPath&,
                        const SkMatrix*,
                        const SkPaint&) override;
  void onDrawTextRSXform(const void*,
                         size_t,
                         const SkRSXform[],
                         const SkRect*,
                         const SkPaint&) override;
  void onDrawTextBlob(const SkTextBlob*,
                      SkScalar,
                      SkScalar,
                      const SkPaint&) override;

  void onDrawBitmap(const SkBitmap&,
                    SkScalar,
                    SkScalar,
                    const SkPaint*) override;
  /*
  void onDrawBitmapLattice(const SkBitmap&,
                           const SkCanvas::Lattice&,
                           const SkRect&,
                           const SkPaint*) override;
  void onDrawBitmapNine(const SkBitmap&,
                        const SkIRect&,
                        const SkRect&,
                        const SkPaint*) override;
  void onDrawBitmapRect(const SkBitmap&,
                        const SkRect*,
                        const SkRect&,
                        const SkPaint*,
                        SkCanvas::SrcRectConstraint) override;
  */
  void onDrawImage(const SkImage*, SkScalar, SkScalar, const SkPaint*) override;
  void onDrawImage(const SkImage*,
                   SkScalar,
                   SkScalar,
                   const CdlPaint&) override;
  /*
  void onDrawImageLattice(const SkImage*,
                          const SkCanvas::Lattice&,
                          const SkRect&,
                          const SkPaint*) override;
  void onDrawImageNine(const SkImage*,
                       const SkIRect&,
                       const SkRect&,
                       const SkPaint*) override;
                       */
  void onDrawImageRect(const SkImage*,
                       const SkRect*,
                       const SkRect&,
                       const SkPaint*,
                       SkCanvas::SrcRectConstraint) override;
  void onDrawImageRect(const SkImage*,
                       const SkRect*,
                       const SkRect&,
                       const CdlPaint&,
                       SkCanvas::SrcRectConstraint) override;

  /*
  void onDrawPatch(const SkPoint[12],
                   const SkColor[4],
                   const SkPoint[4],
                   SkBlendMode,
                   const SkPaint&) override;
                   */
  void onDrawPoints(SkCanvas::PointMode,
                    size_t count,
                    const SkPoint pts[],
                    const SkPaint&) override;
/*
void onDrawVertices(SkCanvas::VertexMode,
                    int,
                    const SkPoint[],
                    const SkPoint[],
                    const SkColor[],
                    SkBlendMode,
                    const uint16_t[],
                    int,
                    const SkPaint&) override;
void onDrawAtlas(const SkImage*,
                 const SkRSXform[],
                 const SkRect[],
                 const SkColor[],
                 int,
                 SkBlendMode,
                 const SkRect*,
                 const SkPaint*) override;
                 */

#ifdef SK_EXPERIMENTAL_SHADOWING
  void didTranslateZ(SkScalar) override;
  void onDrawShadowedPicture(const SkPicture*,
                             const SkMatrix*,
                             const SkPaint*,
                             const SkShadowParams& params) override;
#else
  void didTranslateZ(SkScalar);
  void onDrawShadowedPicture(const SkPicture*,
                             const SkMatrix*,
                             const SkPaint*,
                             const SkShadowParams& params);
#endif

 private:
  CdlLiteDL* fDL;
};

#endif  // SKIA_EXT_CDL_LITE_RECORDER_H_
