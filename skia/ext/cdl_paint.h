/*
 * Copyright 2016 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef SKIA_EXT_CDL_PAINT_H_
#define SKIA_EXT_CDL_PAINT_H_

#include "cdl_common.h"

#if CDL_ENABLED

#include "cdl_shader.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkColorFilter.h"
#include "third_party/skia/include/core/SkDrawLooper.h"
#include "third_party/skia/include/core/SkImageFilter.h"
#include "third_party/skia/include/core/SkMaskFilter.h"
#include "third_party/skia/include/core/SkPaint.h"
#include "third_party/skia/include/core/SkPathEffect.h"
#include "third_party/skia/include/core/SkTypeface.h"

class SK_API CdlPaint {
 public:
  CdlPaint();
  ~CdlPaint();
  CdlPaint(const CdlPaint& paint);

  SkPaint toSkPaint() const;

  enum Style {
    kFill_Style = SkPaint::kFill_Style,
    kStroke_Style = SkPaint::kStroke_Style,
    kStrokeAndFill_Style = SkPaint::kStrokeAndFill_Style,
  };
  Style getStyle() const { return (Style)paint_.getStyle(); }
  void setStyle(Style style) { paint_.setStyle((SkPaint::Style)style); }

  SkColor getColor() const { return paint_.getColor(); }
  void setColor(SkColor color) { paint_.setColor(color); }
  void setARGB(U8CPU a, U8CPU r, U8CPU g, U8CPU b) {
    paint_.setARGB(a, r, g, b);
  }

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

  enum Hinting {
    kNo_Hinting = SkPaint::kNo_Hinting,
    kSlight_Hinting = SkPaint::kSlight_Hinting,
    kNormal_Hinting = SkPaint::kNormal_Hinting,  //!< this is the default
    kFull_Hinting = SkPaint::kFull_Hinting
  };

  Hinting getHinting() const {
    return static_cast<Hinting>(paint_.getHinting());
  }
  void setHinting(Hinting hintingLevel) {
    paint_.setHinting(static_cast<SkPaint::Hinting>(hintingLevel));
  }

  bool isAutohinted() const { return paint_.isAutohinted(); }
  void setAutohinted(bool useAutohinter) {
    paint_.setAutohinted(useAutohinter);
  }

  bool isDither() const { return paint_.isDither(); }
  void setDither(bool dither) { paint_.setDither(dither); }

  enum TextEncoding {
    kUTF8_TextEncoding =
        SkPaint::kUTF8_TextEncoding,  //!< the text parameters are UTF8
    kUTF16_TextEncoding =
        SkPaint::kUTF16_TextEncoding,  //!< the text parameters are UTF16
    kUTF32_TextEncoding =
        SkPaint::kUTF32_TextEncoding,  //!< the text parameters are UTF32
    kGlyphID_TextEncoding = SkPaint::kGlyphID_TextEncoding  //!< the text
                                                            //! parameters are
    //! glyph indices
  };

  TextEncoding getTextEncoding() const {
    return static_cast<TextEncoding>(paint_.getTextEncoding());
  }
  void setTextEncoding(TextEncoding encoding) {
    paint_.setTextEncoding(static_cast<SkPaint::TextEncoding>(encoding));
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

  enum Cap {
    kButt_Cap = SkPaint::kButt_Cap,    //!< begin/end contours with no extension
    kRound_Cap = SkPaint::kRound_Cap,  //!< begin/end contours with a
                                       //! semi-circle extension
    kSquare_Cap = SkPaint::kSquare_Cap,  //!< begin/end contours with a half
                                         //! square extension

    kLast_Cap = kSquare_Cap,
    kDefault_Cap = kButt_Cap
  };
  Cap getStrokeCap() const { return static_cast<Cap>(paint_.getStrokeCap()); }
  void setStrokeCap(Cap cap) {
    paint_.setStrokeCap(static_cast<SkPaint::Cap>(cap));
  }

  enum Join {
    kMiter_Join =
        SkPaint::kMiter_Join,  //!< connect path segments with a sharp join
    kRound_Join =
        SkPaint::kRound_Join,  //!< connect path segments with a round join
    kBevel_Join =
        SkPaint::kBevel_Join,  //!< connect path segments with a flat bevel join

    kLast_Join = kBevel_Join,
    kDefault_Join = kMiter_Join
  };
  Join getStrokeJoin() const {
    return static_cast<Join>(paint_.getStrokeJoin());
  }
  void setStrokeJoin(Join join) {
    paint_.setStrokeJoin(static_cast<SkPaint::Join>(join));
  }

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

inline SkPaint ToSkPaint(const CdlPaint& paint) {
  return paint.toSkPaint();
}

#else  // CDL_ENABLED

#include "third_party/skia/include/core/SkPaint.h"

inline SkPaint ToSkPaint(const CdlPaint& paint) {
  return paint;
}

#endif  // CDL_ENABLED

#endif  // SKIA_EXT_CDL_LITE_RECORDER_H_
