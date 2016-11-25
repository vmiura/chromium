/*
 * Copyright 2016 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef SKIA_EXT_CDL_CANVAS_H_
#define SKIA_EXT_CDL_CANVAS_H_

#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkColorFilter.h"
#include "third_party/skia/include/core/SkDrawLooper.h"
#include "third_party/skia/include/core/SkImageFilter.h"
#include "third_party/skia/include/core/SkPaint.h"
#include "third_party/skia/include/core/SkPathEffect.h"
#include "third_party/skia/include/utils/SkNWayCanvas.h"

class CdlPaint;
class CdlCanvas;
class CdlLiteDL;

class CdlCanvas : public SkRefCnt /*: public SkCanvas*/ {
 public:
  static sk_sp<CdlCanvas> Make(SkCanvas* canvas);

  CdlCanvas();
  explicit CdlCanvas(SkCanvas* canvas);
  CdlCanvas(int width, int height);
  ~CdlCanvas() override;

  SkCanvas* skCanvas();

  // Save / Restore
  int save();
  void restore();

  int saveLayer(const SkRect* bounds, const SkPaint* paint);
  int saveLayer(const SkRect& bounds, const SkPaint* paint) {
    return this->saveLayer(&bounds, paint);
  }
  int saveLayer(const SkCanvas::SaveLayerRec& origRec);
  int saveLayerAlpha(const SkRect* bounds, U8CPU alpha);
  int saveLayerPreserveLCDTextRequests(const SkRect* bounds,
                                     const SkPaint* paint);

  int getSaveCount() const;
  void restoreToCount(int saveCount);

  // Transform
  void concat(const SkMatrix& matrix);
  void rotate(SkScalar degrees);
  void rotate(SkScalar degrees, SkScalar px, SkScalar py);
  void scale(SkScalar sx, SkScalar sy);
  void translate(SkScalar dx, SkScalar dy);

  void resetMatrix();
  void setMatrix(const SkMatrix& matrix);
  const SkMatrix& getTotalMatrix() const;

  // Clip
  SkISize getBaseLayerSize() const;
  bool getClipBounds(SkRect* bounds) const;
  bool getClipDeviceBounds(SkIRect* bounds) const;
  const SkClipStack* getClipStack() const;
  bool isClipEmpty() const;
  bool isClipRect() const;
  bool quickReject(const SkRect& rect) const;

  void clipRect(const SkRect& rect, SkCanvas::ClipOp, bool doAntiAlias);
  void clipRect(const SkRect& rect, SkCanvas::ClipOp op) {
    this->clipRect(rect, op, false);
  }
  void clipRect(const SkRect& rect, bool doAntiAlias = false) {
    this->clipRect(rect, SkCanvas::kIntersect_Op, doAntiAlias);
  }

  void clipRRect(const SkRRect& rrect, SkCanvas::ClipOp op, bool doAntiAlias);
  void clipRRect(const SkRRect& rrect, SkCanvas::ClipOp op) {
    this->clipRRect(rrect, op, false);
  }
  void clipRRect(const SkRRect& rrect, bool doAntiAlias = false) {
    this->clipRRect(rrect, SkCanvas::kIntersect_Op, doAntiAlias);
  }

  void clipPath(const SkPath& path, SkCanvas::ClipOp op, bool doAntiAlias);
  void clipPath(const SkPath& path, SkCanvas::ClipOp op) {
    this->clipPath(path, op, false);
  }
  void clipPath(const SkPath& path, bool doAntiAlias = false) {
    this->clipPath(path, SkCanvas::kIntersect_Op, doAntiAlias);
  }

  void clipRegion(const SkRegion& deviceRgn,
                  SkCanvas::ClipOp op = SkCanvas::kIntersect_Op);

  // Draw
  void drawColor(SkColor color, SkBlendMode mode = SkBlendMode::kSrcOver);
  void drawPaint(const SkPaint& paint);
  void drawPoint(SkScalar x, SkScalar y, const SkPaint& paint);
  void drawPoint(SkScalar x, SkScalar y, SkColor color);
  void drawPoints(SkCanvas::PointMode mode,
                  size_t count,
                  const SkPoint pts[],
                  const SkPaint& paint);
  
  void drawLine(SkScalar x0,
                SkScalar y0,
                SkScalar x1,
                SkScalar y1,
                const SkPaint& paint);
  
  void drawCircle(SkScalar cx,
                  SkScalar cy,
                  SkScalar radius,
                  const SkPaint& paint);
  void drawOval(const SkRect& oval, const SkPaint&);
  
  void drawRect(const SkRect&, const SkPaint&);
  void drawRect(const SkRect&, const CdlPaint&);
  void drawRoundRect(const SkRect& rect,
                     SkScalar rx,
                     SkScalar ry,
                     const SkPaint& paint);
  void drawRectCoords(SkScalar left,
                      SkScalar top,
                      SkScalar right,
                      SkScalar bottom,
                      const SkPaint& paint);
  void drawRRect(const SkRRect& rrect, const SkPaint& paint);
  void drawDRRect(const SkRRect& outer, const SkRRect& inner, const SkPaint&);
  void drawIRect(const SkIRect& rect, const SkPaint& paint) {
    SkRect r;
    r.set(rect);  // promotes the ints to scalars
    this->drawRect(r, paint);
  }

  void drawPath(const SkPath& path, const SkPaint& paint);

  void drawBitmap(const SkBitmap& bitmap,
                  SkScalar left,
                  SkScalar top,
                  const SkPaint* paint = NULL);

  void drawImage(const SkImage* image,
                 SkScalar left,
                 SkScalar top,
                 const SkPaint* paint = NULL);
  void drawImage(const sk_sp<SkImage>& image,
                 SkScalar left,
                 SkScalar top,
                 const SkPaint* paint = NULL) {
    this->drawImage(image.get(), left, top, paint);
  }

  void drawImage(const SkImage* image,
                 SkScalar left,
                 SkScalar top,
                 const CdlPaint& paint);
  void drawImage(const sk_sp<SkImage>& image,
                 SkScalar left,
                 SkScalar top,
                 const CdlPaint& paint) {
    this->drawImage(image.get(), left, top, paint);
  }

  void drawImageRect(const SkImage* image,
                     const SkRect& src,
                     const SkRect& dst,
                     const SkPaint* paint,
                     SkCanvas::SrcRectConstraint constraint =
                         SkCanvas::kStrict_SrcRectConstraint);
  void drawImageRect(const SkImage* image,
                     const SkRect& src,
                     const SkRect& dst,
                     const CdlPaint& paint,
                     SkCanvas::SrcRectConstraint constraint =
                         SkCanvas::kStrict_SrcRectConstraint);

  // variant that takes src SkIRect
  void drawImageRect(
      const SkImage* image,
      const SkIRect& isrc,
      const SkRect& dst,
      const SkPaint* paint,
      SkCanvas::SrcRectConstraint = SkCanvas::kStrict_SrcRectConstraint);
  // variant that assumes src == image-bounds
  void drawImageRect(
      const SkImage* image,
      const SkRect& dst,
      const SkPaint* paint,
      SkCanvas::SrcRectConstraint = SkCanvas::kStrict_SrcRectConstraint);

  void drawImageRect(const sk_sp<SkImage>& image,
                     const SkRect& src,
                     const SkRect& dst,
                     const SkPaint* paint,
                     SkCanvas::SrcRectConstraint constraint =
                         SkCanvas::kStrict_SrcRectConstraint) {
    this->drawImageRect(image.get(), src, dst, paint, constraint);
  }
  void drawImageRect(
      const sk_sp<SkImage>& image,
      const SkIRect& isrc,
      const SkRect& dst,
      const SkPaint* paint,
      SkCanvas::SrcRectConstraint cons = SkCanvas::kStrict_SrcRectConstraint) {
    this->drawImageRect(image.get(), isrc, dst, paint, cons);
  }
  void drawImageRect(
      const sk_sp<SkImage>& image,
      const SkRect& dst,
      const SkPaint* paint,
      SkCanvas::SrcRectConstraint cons = SkCanvas::kStrict_SrcRectConstraint) {
    this->drawImageRect(image.get(), dst, paint, cons);
  }

  void drawText(const void* text,
                size_t byteLength,
                SkScalar x,
                SkScalar y,
                const SkPaint& paint);
  void drawPosText(const void* text,
                   size_t byteLength,
                   const SkPoint pos[],
                   const SkPaint& paint);
  void drawTextBlob(const SkTextBlob* blob,
                    SkScalar x,
                    SkScalar y,
                    const SkPaint& paint);
  void drawTextBlob(const sk_sp<SkTextBlob>& blob,
                    SkScalar x,
                    SkScalar y,
                    const SkPaint& paint) {
    this->drawTextBlob(blob.get(), x, y, paint);
  }

  void drawDrawable(SkDrawable* drawable, const SkMatrix* = NULL);
  void drawDrawable(SkDrawable*, SkScalar x, SkScalar y);

  // Misc
  void flush();
  void discard() { this->onDiscard(); }
  void clear(SkColor color) { this->drawColor(color, SkBlendMode::kSrc); }
  bool readPixels(SkBitmap* bitmap, int srcX, int srcY);
  bool writePixels(const SkImageInfo&,
                   const void* pixels,
                   size_t rowBytes,
                   int x,
                   int y);
  bool writePixels(const SkBitmap& bitmap, int x, int y);

 protected:

  enum SaveLayerStrategy {
    kFullLayer_SaveLayerStrategy,
    kNoLayer_SaveLayerStrategy,
  };

  virtual int  onSave();
  virtual int  onSaveLayer(const SkCanvas::SaveLayerRec&);
  virtual void onRestore();

  virtual void onConcat(const SkMatrix&);
  virtual void onSetMatrix(const SkMatrix&);
  virtual void onTranslate(SkScalar, SkScalar);

  enum ClipEdgeStyle { kHard_ClipEdgeStyle, kSoft_ClipEdgeStyle };

  virtual void onClipRect(const SkRect&, SkCanvas::ClipOp, ClipEdgeStyle);
  virtual void onClipRRect(const SkRRect&, SkCanvas::ClipOp, ClipEdgeStyle);
  virtual void onClipPath(const SkPath&, SkCanvas::ClipOp, ClipEdgeStyle);
  virtual void onClipRegion(const SkRegion&, SkCanvas::ClipOp);

  virtual void onDiscard();

  virtual void onDrawPaint(const SkPaint&);
  virtual void onDrawPath(const SkPath&, const SkPaint&);
  virtual void onDrawRect(const SkRect&, const SkPaint&);
  virtual void onDrawRect(const SkRect&, const CdlPaint&);
  virtual void onDrawRegion(const SkRegion&, const SkPaint&);
  virtual void onDrawOval(const SkRect&, const SkPaint&);
  virtual void onDrawArc(const SkRect&,
                         SkScalar,
                         SkScalar,
                         bool,
                         const SkPaint&);
  virtual void onDrawRRect(const SkRRect&, const SkPaint&);
  virtual void onDrawDRRect(const SkRRect&, const SkRRect&, const SkPaint&);

  virtual void onDrawDrawable(SkDrawable*, const SkMatrix*);
  /*
  virtual void onDrawPicture(const SkPicture*,
                     const SkMatrix*,
                     const SkPaint*);
                     */
  virtual void onDrawAnnotation(const SkRect&, const char[], SkData*);

  virtual void onDrawText(const void*,
                          size_t,
                          SkScalar x,
                          SkScalar y,
                          const SkPaint&);
  virtual void onDrawPosText(const void*,
                             size_t,
                             const SkPoint[],
                             const SkPaint&);
  virtual void onDrawPosTextH(const void*,
                              size_t,
                              const SkScalar[],
                              SkScalar,
                              const SkPaint&);
  virtual void onDrawTextOnPath(const void*,
                                size_t,
                                const SkPath&,
                                const SkMatrix*,
                                const SkPaint&);
  virtual void onDrawTextRSXform(const void*,
                                 size_t,
                                 const SkRSXform[],
                                 const SkRect*,
                                 const SkPaint&);
  virtual void onDrawTextBlob(const SkTextBlob*,
                              SkScalar,
                              SkScalar,
                              const SkPaint&);

  virtual void onDrawBitmap(const SkBitmap&,
                            SkScalar,
                            SkScalar,
                            const SkPaint*);
  /*
  virtual void onDrawBitmapLattice(const SkBitmap&,
                           const SkCanvas::Lattice&,
                           const SkRect&,
                           const SkPaint*);
  virtual void onDrawBitmapNine(const SkBitmap&,
                        const SkIRect&,
                        const SkRect&,
                        const SkPaint*);
  virtual void onDrawBitmapRect(const SkBitmap&,
                        const SkRect*,
                        const SkRect&,
                        const SkPaint*,
                        SkCanvas::SrcRectConstraint);
  */
  virtual void onDrawImage(const SkImage*, SkScalar, SkScalar, const SkPaint*);
  virtual void onDrawImage(const SkImage*, SkScalar, SkScalar, const CdlPaint&);
  /*
  virtual void onDrawImageLattice(const SkImage*,
                          const SkCanvas::Lattice&,
                          const SkRect&,
                          const SkPaint*);
  virtual void onDrawImageNine(const SkImage*,
                       const SkIRect&,
                       const SkRect&,
                       const SkPaint*);
                    */
  virtual void onDrawImageRect(const SkImage*,
                               const SkRect*,
                               const SkRect&,
                               const SkPaint*,
                               SkCanvas::SrcRectConstraint);
  virtual void onDrawImageRect(const SkImage*,
                               const SkRect*,
                               const SkRect&,
                               const CdlPaint&,
                               SkCanvas::SrcRectConstraint);

  /*
  virtual void onDrawPatch(const SkPoint[12],
                   const SkColor[4],
                   const SkPoint[4],
                   SkBlendMode,
                   const SkPaint&);
                   */
  virtual void onDrawPoints(SkCanvas::PointMode,
                            size_t count,
                            const SkPoint pts[],
                            const SkPaint&);
/*
virtual void onDrawVertices(SkCanvas::VertexMode,
                    int,
                    const SkPoint[],
                    const SkPoint[],
                    const SkColor[],
                    SkBlendMode,
                    const uint16_t[],
                    int,
                    const SkPaint&);

virtual void onDrawAtlas(const SkImage*,
                 const SkRSXform[],
                 const SkRect[],
                 const SkColor[],
                 int,
                 SkBlendMode,
                 const SkRect*,
                 const SkPaint*);
                 */
  

  SkCanvas* canvas_;
};

class CdlAutoCanvasRestore : SkNoncopyable {
 public:
  CdlAutoCanvasRestore(CdlCanvas* canvas, bool doSave)
      : fCanvas(canvas), fSaveCount(0) {
    if (fCanvas) {
      fSaveCount = canvas->getSaveCount();
      if (doSave) {
        canvas->save();
      }
    }
  }
  ~CdlAutoCanvasRestore() {
    if (fCanvas) {
      fCanvas->restoreToCount(fSaveCount);
    }
  }

  /**
   *  Perform the restore now, instead of waiting for the destructor. Will
   *  only do this once.
   */
  void restore() {
    if (fCanvas) {
      fCanvas->restoreToCount(fSaveCount);
      fCanvas = NULL;
    }
  }

 private:
  CdlCanvas* fCanvas;
  int fSaveCount;
};

#endif  // SKIA_EXT_CDL_CANVAS_H_
