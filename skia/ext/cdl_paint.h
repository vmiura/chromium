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
#include "third_party/skia/include/core/SkTypeface.h"

class CdlPaint {
 public:
  CdlPaint();
  ~CdlPaint();
  CdlPaint(const CdlPaint& paint);

  SkPaint toSkPaint() const;

  SkPaint::Style getStyle() const { return paint_.getStyle(); }
  void setStyle(SkPaint::Style style) { paint_.setStyle(style); }

  SkColor getColor() const { return paint_.getColor(); }
  void setColor(SkColor color) { paint_.setColor(color); }

  uint8_t getAlpha() const { return paint_.getAlpha(); }
  void setAlpha(U8CPU a) { paint_.setAlpha(a); }

  void setBlendMode(SkBlendMode mode) { paint_.setBlendMode(mode); }
  SkBlendMode getBlendMode() const { return paint_.getBlendMode(); }
  bool isSrcOver() const { return paint_.isSrcOver(); }

  bool isAntiAlias() const { return paint_.isAntiAlias(); }
  void setAntiAlias(bool aa) { paint_.setAntiAlias(aa); }

  bool isVerticalText() const { return paint_.isVerticalText(); }
  void setVerticalText(bool vertical) { paint_.setVerticalText(vertical); }

  bool isSubpixelText() const { return paint_.isSubpixelText(); }
  void setSubpixelText(bool subpixelText) {
    paint_.setSubpixelText(subpixelText);
  }

  bool isLCDRenderText() const { return paint_.isLCDRenderText(); }
  void setLCDRenderText(bool lcdText) { paint_.setLCDRenderText(lcdText); }

  SkPaint::Hinting getHinting() const { return paint_.getHinting(); }
  void setHinting(SkPaint::Hinting hintingLevel) {
    paint_.setHinting(hintingLevel);
  }

  bool isAutohinted() const { return paint_.isAutohinted(); }
  void setAutohinted(bool useAutohinter) {
    paint_.setAutohinted(useAutohinter);
  }

  bool isDither() const { return paint_.isDither(); }
  void setDither(bool dither) { paint_.setDither(dither); }

  SkPaint::TextEncoding getTextEncoding() const {
    return paint_.getTextEncoding();
  }
  void setTextEncoding(SkPaint::TextEncoding encoding) {
    paint_.setTextEncoding(encoding);
  }

  SkScalar getTextSize() const { return paint_.getTextSize(); }
  void setTextSize(SkScalar textSize) { paint_.setTextSize(textSize); }

  void setFilterQuality(SkFilterQuality quality) {
    paint_.setFilterQuality(quality);
  }
  SkFilterQuality getFilterQuality() const { return paint_.getFilterQuality(); }

  SkScalar getStrokeWidth() const { return paint_.getStrokeWidth(); }
  void setStrokeWidth(SkScalar width) { paint_.setStrokeWidth(width); }

  SkScalar getStrokeMiter() const { return paint_.getStrokeMiter(); }
  void setStrokeMiter(SkScalar miter) { paint_.setStrokeMiter(miter); }

  SkPaint::Cap getStrokeCap() const { return paint_.getStrokeCap(); }
  void setStrokeCap(SkPaint::Cap cap) { paint_.setStrokeCap(cap); }

  SkPaint::Join getStrokeJoin() const { return paint_.getStrokeJoin(); }
  void setStrokeJoin(SkPaint::Join join) { paint_.setStrokeJoin(join); }

  SkTypeface* getTypeface() const { return paint_.getTypeface(); }
  void setTypeface(sk_sp<SkTypeface> typeface) { paint_.setTypeface(typeface); }

  SkColorFilter* getColorFilter() const { return paint_.getColorFilter(); }
  void setColorFilter(sk_sp<SkColorFilter> filter) {
    paint_.setColorFilter(filter);
  }

  SkMaskFilter* getMaskFilter() const { return paint_.getMaskFilter(); }
  void setMaskFilter(sk_sp<SkMaskFilter> mask) { paint_.setMaskFilter(mask); };

  CdlShader* getShader() const { return shader_.get(); }
  void setShader(sk_sp<CdlShader> shader) { shader_ = shader; }

  SkPathEffect* getPathEffect() const { return paint_.getPathEffect(); }
  void setPathEffect(sk_sp<SkPathEffect> effect) {
    paint_.setPathEffect(effect);
  }

  SkImageFilter* getImageFilter() const { return paint_.getImageFilter(); }
  void setImageFilter(sk_sp<SkImageFilter> filter) {
    paint_.setImageFilter(filter);
  }

  SkDrawLooper* getDrawLooper() const { return paint_.getDrawLooper(); }
  SkDrawLooper* getLooper() const { return paint_.getLooper(); }
  void setLooper(sk_sp<SkDrawLooper> looper) { paint_.setLooper(looper); }

  bool canComputeFastBounds() const { return paint_.canComputeFastBounds(); }
  const SkRect& computeFastBounds(const SkRect& orig, SkRect* storage) const {
    return paint_.computeFastBounds(orig, storage);
  }

 protected:
  SkPaint paint_;
  sk_sp<CdlShader> shader_;
};

#endif  // SKIA_EXT_CDL_LITE_RECORDER_H_
