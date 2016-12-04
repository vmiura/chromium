/*
 * Copyright 2016 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef SKIA_EXT_CDL_PAINT_H_
#define SKIA_EXT_CDL_PAINT_H_

#include "cdl_shader.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkColorFilter.h"
#include "third_party/skia/include/core/SkDrawLooper.h"
#include "third_party/skia/include/core/SkImageFilter.h"
#include "third_party/skia/include/core/SkMaskFilter.h"
#include "third_party/skia/include/core/SkPaint.h"
#include "third_party/skia/include/core/SkPathEffect.h"

class CdlPaint {
 public:
  CdlPaint();
  ~CdlPaint();
  CdlPaint(const CdlPaint& paint);
  explicit CdlPaint(const SkPaint& paint);
  SkPaint toSkPaint() const;

  void setStyle(SkPaint::Style style) { sk_paint.setStyle(style); }
  SkPaint::Style getStyle() const { return sk_paint.getStyle(); }

  void setColor(SkColor color) {
    sk_paint.setColor(color);
    is_dirty_ = true;
  }
  SkColor getColor() const { return sk_paint.getColor(); }

  void setAlpha(U8CPU a) { sk_paint.setAlpha(a); }
  uint8_t getAlpha() const { return sk_paint.getAlpha(); }

  void setBlendMode(SkBlendMode mode) { sk_paint.setBlendMode(mode); }
  SkBlendMode getBlendMode() const { return sk_paint.getBlendMode(); }
  bool isSrcOver() const { return sk_paint.isSrcOver(); }

  bool isAntiAlias() const { return sk_paint.isAntiAlias(); }
  void setAntiAlias(bool aa) { sk_paint.setAntiAlias(aa); }

  void setDither(bool dither) { sk_paint.setDither(dither); }

  void setFilterQuality(SkFilterQuality quality) {
    sk_paint.setFilterQuality(quality);
  }
  SkFilterQuality getFilterQuality() const {
    return sk_paint.getFilterQuality();
  }

  void setStrokeWidth(SkScalar width) { sk_paint.setStrokeWidth(width); }
  SkScalar getStrokeWidth() const { return sk_paint.getStrokeWidth(); }

  void setStrokeMiter(SkScalar miter) { sk_paint.setStrokeMiter(miter); }
  SkScalar getStrokeMiter() const { return sk_paint.getStrokeMiter(); }

  void setStrokeCap(SkPaint::Cap cap) { sk_paint.setStrokeCap(cap); }
  SkPaint::Cap getStrokeCap() const { return sk_paint.getStrokeCap(); }

  void setStrokeJoin(SkPaint::Join join) { sk_paint.setStrokeJoin(join); }
  SkPaint::Join getStrokeJoin() const { return sk_paint.getStrokeJoin(); }

  void setColorFilter(sk_sp<SkColorFilter> filter) {
    sk_paint.setColorFilter(filter);
  }
  void setLooper(sk_sp<SkDrawLooper> looper) { sk_paint.setLooper(looper); }

  void setMaskFilter(sk_sp<SkMaskFilter> mask) {
    sk_paint.setMaskFilter(mask);
  };
  SkMaskFilter* getMaskFilter() const { return sk_paint.getMaskFilter(); }

  void setShader(sk_sp<SkShader> shader) { sk_paint.setShader(shader); }
  SkShader* getShader() const { return sk_paint.getShader(); }

  void setCdlShader(sk_sp<CdlShader> shader) {
    shader_ = shader;
    is_dirty_ = true;
  }

  CdlShader* getCdlShader() const { return shader_.get(); }

  void setPathEffect(sk_sp<SkPathEffect> effect) {
    sk_paint.setPathEffect(effect);
  }
  SkPathEffect* getPathEffect() const { return sk_paint.getPathEffect(); }

  bool getFillPath(const SkPath& src,
                   SkPath* dst,
                   const SkRect* cullRect,
                   SkScalar resScale = 1) {
    return sk_paint.getFillPath(src, dst, cullRect, resScale);
  };

  SkColorFilter* getColorFilter() const { return sk_paint.getColorFilter(); }

  SkImageFilter* getImageFilter() const { return sk_paint.getImageFilter(); }
  void setImageFilter(sk_sp<SkImageFilter> filter) {
    sk_paint.setImageFilter(filter);
  }

  SkDrawLooper* getDrawLooper() const { return sk_paint.getDrawLooper(); }
  SkDrawLooper* getLooper() const { return sk_paint.getLooper(); }

  int getTextBlobIntercepts(const SkTextBlob* blob,
                            const SkScalar bounds[2],
                            SkScalar* intervals) const {
    return sk_paint.getTextBlobIntercepts(blob, bounds, intervals);
  }

 protected:
  mutable SkPaint sk_paint;
  mutable bool is_dirty_;
  sk_sp<CdlShader> shader_;
};

#endif  // SKIA_EXT_CDL_LITE_RECORDER_H_
