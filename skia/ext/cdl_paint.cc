/*
 * Copyright 2016 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <cstdlib>
#include "cdl_paint.h"

#include "base/logging.h"
#include "cdl_shader.h"

CdlPaint::CdlPaint() : is_dirty_(true) {}
CdlPaint::~CdlPaint() {}

CdlPaint::CdlPaint(const CdlPaint& paint)
    : sk_paint(paint.sk_paint),
      is_dirty_(paint.is_dirty_),
      shader_(paint.shader_) {}

CdlPaint::CdlPaint(const SkPaint& paint) : sk_paint(paint), is_dirty_(false) {}

SkPaint CdlPaint::toSkPaint() const {
  if (is_dirty_) {
    is_dirty_ = false;
    if (shader_.get()) {
      sk_paint.setShader(shader_->createSkShader());
    }

    // Change to rainbow colors to show invalidations.
    // sk_paint.setColor((sk_paint.getColor() & 0xff777777) + rand() %
    // 0x777777);
  }

  return sk_paint;
}
