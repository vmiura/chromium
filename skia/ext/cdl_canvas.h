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


class CdlPaint;
class CdlCanvas;
class CdlLiteDL;

class CdlCanvas : public SkCanvas {
 public:
  explicit CdlCanvas(SkCanvas* canvas);
  CdlCanvas(int width, int height);
  ~CdlCanvas() override;

  using SkCanvas::drawRect;

  void drawRect(const SkRect&, const CdlPaint&);


 protected:
  using SkCanvas::onDrawRect;
  virtual void onDrawRect(const SkRect&, const CdlPaint&);
};

#endif  // SKIA_EXT_CDL_CANVAS_H_
