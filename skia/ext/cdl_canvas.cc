/*
 * Copyright 2016 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "cdl_canvas.h"
#include "third_party/skia/include/core/SkCanvas.h"

CdlCanvas::CdlCanvas(SkCanvas* canvas)
 : SkCanvas(canvas->getDevice()) {}

CdlCanvas::CdlCanvas(int width, int height)
 : SkCanvas(width, height) {}

CdlCanvas::~CdlCanvas() {}

void CdlCanvas::drawRect(const SkRect& r, const CdlPaint& paint) {
  onDrawRect(r, paint);
}

void CdlCanvas::onDrawRect(const SkRect&, const CdlPaint&) {

}
