/*
 * Copyright 2016 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "cdl_paint.h"

#if CDL_ENABLED

#include <cstdlib>

#include "base/logging.h"
#include "cdl_shader.h"

CdlPaint::CdlPaint() {}
CdlPaint::~CdlPaint() {}

CdlPaint::CdlPaint(const CdlPaint& paint)
    : paint_(paint.paint_), shader_(paint.shader_) {}

SkPaint CdlPaint::toSkPaint() const {
  SkPaint paint = paint_;
  if (shader_.get())
    paint.setShader(shader_->createSkShader());
  return paint;
}

#endif  // CDL_ENABLED
