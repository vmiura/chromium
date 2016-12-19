/*
 * Copyright 2016 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef SKIA_EXT_CDL_CANVAS_H_
#define SKIA_EXT_CDL_CANVAS_H_

#include "cdl_common.h"

#if CDL_ENABLED

#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkColorFilter.h"
#include "third_party/skia/include/core/SkDrawLooper.h"
#include "third_party/skia/include/core/SkImageFilter.h"
#include "third_party/skia/include/core/SkPaint.h"
#include "third_party/skia/include/core/SkPathEffect.h"
#include "third_party/skia/include/utils/SkNWayCanvas.h"

class CdlPaint;
class CdlPicture;

class SK_API CdlCanvas : public SkRefCnt {
 public:
  static sk_sp<CdlCanvas> Make(SkCanvas* canvas);

  CdlCanvas();
  explicit CdlCanvas(SkCanvas* canvas);
  explicit CdlCanvas(const SkBitmap& bitmap);
  explicit CdlCanvas(SkBaseDevice* device);
  CdlCanvas(int width, int height);
  CdlCanvas(const SkBitmap& bitmap, const SkSurfaceProps& props);
  ~CdlCanvas() override;

  SkCanvas* skCanvas() { return canvas_; }
  const SkCanvas* skCanvas() const { return canvas_; }

  // Save / Restore
  int save();
  void restore();

  typedef uint32_t SaveLayerFlags;

  struct SaveLayerRec {
    SaveLayerRec()
        : fBounds(nullptr),
          fPaint(nullptr),
          fBackdrop(nullptr),
          fSaveLayerFlags(0) {}
    SaveLayerRec(const SkRect* bounds,
                 const CdlPaint* paint,
                 SaveLayerFlags saveLayerFlags = 0)
        : fBounds(bounds),
          fPaint(paint),
          fBackdrop(nullptr),
          fSaveLayerFlags(saveLayerFlags) {}
    SaveLayerRec(const SkRect* bounds,
                 const CdlPaint* paint,
                 const SkImageFilter* backdrop,
                 SaveLayerFlags saveLayerFlags)
        : fBounds(bounds),
          fPaint(paint),
          fBackdrop(backdrop),
          fSaveLayerFlags(saveLayerFlags) {}

    const SkRect* fBounds;           // optional
    const CdlPaint* fPaint;          // optional
    const SkImageFilter* fBackdrop;  // optional
    SaveLayerFlags fSaveLayerFlags;
  };

  int saveLayer(const SkRect* bounds, const CdlPaint* paint);
  int saveLayer(const SkRect& bounds, const CdlPaint* paint) {
    return this->saveLayer(&bounds, paint);
  }
  int saveLayer(const SaveLayerRec& origRec);
  int saveLayerAlpha(const SkRect* bounds, U8CPU alpha);
  int saveLayerPreserveLCDTextRequests(const SkRect* bounds,
                                       const CdlPaint* paint);

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
  void drawPaint(const CdlPaint& paint);
  void drawPoint(SkScalar x, SkScalar y, const CdlPaint& paint);
  void drawPoint(SkScalar x, SkScalar y, SkColor color);
  void drawPoints(SkCanvas::PointMode mode,
                  size_t count,
                  const SkPoint pts[],
                  const CdlPaint& paint);

  void drawLine(SkScalar x0,
                SkScalar y0,
                SkScalar x1,
                SkScalar y1,
                const CdlPaint& paint);

  void drawCircle(SkScalar cx,
                  SkScalar cy,
                  SkScalar radius,
                  const CdlPaint& paint);
  void drawOval(const SkRect& oval, const CdlPaint&);

  void drawRect(const SkRect&, const CdlPaint&);
  void drawRoundRect(const SkRect& rect,
                     SkScalar rx,
                     SkScalar ry,
                     const CdlPaint& paint);
  void drawRectCoords(SkScalar left,
                      SkScalar top,
                      SkScalar right,
                      SkScalar bottom,
                      const CdlPaint& paint);
  void drawRRect(const SkRRect& rrect, const CdlPaint& paint);
  void drawDRRect(const SkRRect& outer, const SkRRect& inner, const CdlPaint&);
  void drawIRect(const SkIRect& rect, const CdlPaint& paint) {
    SkRect r;
    r.set(rect);  // promotes the ints to scalars
    this->drawRect(r, paint);
  }

  void drawPath(const SkPath& path, const CdlPaint& paint);

  void drawBitmap(const SkBitmap& bitmap,
                  SkScalar left,
                  SkScalar top,
                  const CdlPaint* paint = NULL);

  void drawImage(const SkImage* image,
                 SkScalar left,
                 SkScalar top,
                 const CdlPaint* paint = NULL);
  void drawImage(const sk_sp<SkImage>& image,
                 SkScalar left,
                 SkScalar top,
                 const CdlPaint* paint = NULL) {
    this->drawImage(image.get(), left, top, paint);
  }

  void drawImageRect(const SkImage* image,
                     const SkRect& src,
                     const SkRect& dst,
                     const CdlPaint* paint,
                     SkCanvas::SrcRectConstraint constraint =
                         SkCanvas::kStrict_SrcRectConstraint);
  // variant that takes src SkIRect
  void drawImageRect(
      const SkImage* image,
      const SkIRect& isrc,
      const SkRect& dst,
      const CdlPaint* paint,
      SkCanvas::SrcRectConstraint = SkCanvas::kStrict_SrcRectConstraint);
  // variant that assumes src == image-bounds
  void drawImageRect(
      const SkImage* image,
      const SkRect& dst,
      const CdlPaint* paint,
      SkCanvas::SrcRectConstraint = SkCanvas::kStrict_SrcRectConstraint);

  void drawImageRect(const sk_sp<SkImage>& image,
                     const SkRect& src,
                     const SkRect& dst,
                     const CdlPaint* paint,
                     SkCanvas::SrcRectConstraint constraint =
                         SkCanvas::kStrict_SrcRectConstraint) {
    this->drawImageRect(image.get(), src, dst, paint, constraint);
  }
  void drawImageRect(
      const sk_sp<SkImage>& image,
      const SkIRect& isrc,
      const SkRect& dst,
      const CdlPaint* paint,
      SkCanvas::SrcRectConstraint cons = SkCanvas::kStrict_SrcRectConstraint) {
    this->drawImageRect(image.get(), isrc, dst, paint, cons);
  }
  void drawImageRect(
      const sk_sp<SkImage>& image,
      const SkRect& dst,
      const CdlPaint* paint,
      SkCanvas::SrcRectConstraint cons = SkCanvas::kStrict_SrcRectConstraint) {
    this->drawImageRect(image.get(), dst, paint, cons);
  }

  void drawText(const void* text,
                size_t byteLength,
                SkScalar x,
                SkScalar y,
                const CdlPaint& paint);
  void drawPosText(const void* text,
                   size_t byteLength,
                   const SkPoint pos[],
                   const CdlPaint& paint);
  void drawTextBlob(const SkTextBlob* blob,
                    SkScalar x,
                    SkScalar y,
                    const CdlPaint& paint);
  void drawTextBlob(const sk_sp<SkTextBlob>& blob,
                    SkScalar x,
                    SkScalar y,
                    const CdlPaint& paint) {
    this->drawTextBlob(blob.get(), x, y, paint);
  }

  void drawPicture(const CdlPicture* picture) {
    this->drawPicture(picture, NULL, NULL);
  }
  void drawPicture(const sk_sp<CdlPicture>& picture) {
    this->drawPicture(picture.get());
  }
  void drawPicture(const CdlPicture*,
                   const SkMatrix* matrix,
                   const CdlPaint* paint);
  void drawPicture(const sk_sp<CdlPicture>& picture,
                   const SkMatrix* matrix,
                   const CdlPaint* paint) {
    this->drawPicture(picture.get(), matrix, paint);
  }

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

  virtual int onSave();
  virtual int onSaveLayer(const SaveLayerRec&);
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

  virtual void onDrawPaint(const CdlPaint&);
  virtual void onDrawPath(const SkPath&, const CdlPaint&);
  virtual void onDrawRect(const SkRect&, const CdlPaint&);
  virtual void onDrawOval(const SkRect&, const CdlPaint&);
  virtual void onDrawRRect(const SkRRect&, const CdlPaint&);
  virtual void onDrawDRRect(const SkRRect&, const SkRRect&, const CdlPaint&);

  virtual void onDrawPicture(const CdlPicture* picture,
                             const SkMatrix* matrix,
                             const CdlPaint* paint);
  virtual void onDrawAnnotation(const SkRect&, const char[], SkData*);

  virtual void onDrawText(const void*,
                          size_t,
                          SkScalar x,
                          SkScalar y,
                          const CdlPaint&);
  virtual void onDrawPosText(const void*,
                             size_t,
                             const SkPoint[],
                             const CdlPaint&);
  virtual void onDrawTextBlob(const SkTextBlob*,
                              SkScalar,
                              SkScalar,
                              const CdlPaint&);

  virtual void onDrawImage(const SkImage*, SkScalar, SkScalar, const CdlPaint*);

  virtual void onDrawImageRect(const SkImage*,
                               const SkRect*,
                               const SkRect&,
                               const CdlPaint*,
                               SkCanvas::SrcRectConstraint);

  virtual void onDrawPoints(SkCanvas::PointMode,
                            size_t count,
                            const SkPoint pts[],
                            const CdlPaint&);

  std::unique_ptr<SkCanvas> owned_canvas_;
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

class SK_API CdlPassThroughCanvas : public CdlCanvas {
 public:
  CdlPassThroughCanvas(SkCanvas* canvas);
  ~CdlPassThroughCanvas() override;
};

inline SK_API const SkCanvas* GetSkCanvas(const CdlCanvas* canvas) {
  return canvas->skCanvas();
}
inline SK_API SkCanvas* GetSkCanvas(CdlCanvas* canvas) {
  return canvas->skCanvas();
}

//#define CDL_WRAP_SKCANVAS(x) (CdlCanvas::Make(x).get())

#else

#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/utils/SkNWayCanvas.h"

class SK_API CdlPassThroughCanvas : public SkNWayCanvas {
 public:
  CdlPassThroughCanvas(SkCanvas* canvas);
  ~CdlPassThroughCanvas() override;
};

inline const SK_API SkCanvas* GetSkCanvas(const CdlCanvas* canvas) {
  return canvas;
}
inline SK_API SkCanvas* GetSkCanvas(CdlCanvas* canvas) {
  return canvas;
}

//#define CDL_WRAP_SKCANVAS(x) (x)

#endif  // CDL_ENABLED

#endif  // SKIA_EXT_CDL_CANVAS_H_
