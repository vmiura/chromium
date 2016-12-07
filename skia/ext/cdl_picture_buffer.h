// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SKIA_EXT_CDL_PICTURE_BUFFER_H_
#define SKIA_EXT_CDL_PICTURE_BUFFER_H_

#include <stddef.h>
#include <stdint.h>

#include "base/compiler_specific.h"

#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkPaint.h"
#include "third_party/skia/include/core/SkPath.h"
#include "third_party/skia/include/core/SkDrawable.h"
#include "third_party/skia/include/core/SkRect.h"
#include "third_party/skia/include/private/SkTDArray.h"

class CdlCanvas;
class CdlPaint;
class CdlPicture;

class CdlPictureBuffer : public SkRefCnt /*public SkDrawable*/ {
 public:
  CdlPictureBuffer(SkRect bounds);
  ~CdlPictureBuffer() override;

  // Prepares to append new picture to same record.
  void resetForNextPicture(SkRect bounds);

  // Discards full contents.
  void reset(SkRect);

  void makeThreadsafe();
  bool empty() const { return fUsed == 0; }
  int getRecordOffset() { return fUsed; }

  void playback(CdlCanvas*, int start_offset, int end_offset);

  SkRect getBounds();

  // Draws as if...
  //   SkRect bounds = this->getBounds();
  //   canvas->saveLayer(&bounds, paint);
  //       this->draw(canvas, matrix);
  //   canvas->restore();
  void drawAsLayer(CdlCanvas*, const SkMatrix*, const SkPaint*);

  void save();
  void saveLayer(const SkRect*,
                 const SkPaint*,
                 const SkImageFilter*,
                 SkCanvas::SaveLayerFlags);
  void restore();

  void concat(const SkMatrix&);
  void setMatrix(const SkMatrix&);
  void translate(SkScalar, SkScalar);

  void clipPath(const SkPath&, SkCanvas::ClipOp, bool aa);
  void clipRect(const SkRect&, SkCanvas::ClipOp, bool aa);
  void clipRRect(const SkRRect&, SkCanvas::ClipOp, bool aa);
  void clipRegion(const SkRegion&, SkCanvas::ClipOp);

  void drawPaint(const CdlPaint&);
  void drawPath(const SkPath&, const CdlPaint&);
  void drawRect(const SkRect&, const SkPaint&);
  void drawRect(const SkRect&, const CdlPaint&);
  void drawOval(const SkRect&, const CdlPaint&);
  void drawRRect(const SkRRect&, const SkPaint&);
  void drawDRRect(const SkRRect&, const SkRRect&, const SkPaint&);

  void drawAnnotation(const SkRect&, const char*, SkData*);
  void drawPicture(const CdlPicture*, const SkMatrix*, const SkPaint*);

  void drawText(const void*, size_t, SkScalar, SkScalar, const CdlPaint&);
  void drawPosText(const void*, size_t, const SkPoint[], const CdlPaint&);
  void drawTextBlob(const SkTextBlob*, SkScalar, SkScalar, const CdlPaint&);

  void drawImage(sk_sp<const SkImage>, SkScalar, SkScalar, const SkPaint*);
  void drawImage(sk_sp<const SkImage>, SkScalar, SkScalar, const CdlPaint&);
  void drawImageRect(sk_sp<const SkImage>,
                     const SkRect*,
                     const SkRect&,
                     const SkPaint*,
                     SkCanvas::SrcRectConstraint);
  void drawImageRect(sk_sp<const SkImage>,
                     const SkRect*,
                     const SkRect&,
                     const CdlPaint&,
                     SkCanvas::SrcRectConstraint);
  void drawPoints(SkCanvas::PointMode, size_t, const SkPoint[], const CdlPaint&);
  void setBounds(const SkRect& bounds);

  // Cdl
  struct DrawContext {};

 private:
  // SkRect onGetBounds() override;
  // void onDraw(SkCanvas*) override;

  template <typename T, typename... Args>
  void* push(size_t, Args&&...);

  template <typename Fn, typename... Args>
  void map(const Fn[], int start_offset, int end_offset, Args...);

  SkAutoTMalloc<uint8_t> fBytes;
  size_t fUsed;
  size_t fReserved;
  SkRect fBounds;
};

#endif  // SKIA_EXT_CDL_PICTURE_BUFFER_H_
