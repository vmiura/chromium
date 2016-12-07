// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/graphics/PlaceholderImage.h"

#include "platform/geometry/FloatRect.h"
#include "platform/graphics/Color.h"
#include "platform/graphics/GraphicsContext.h"
#include "platform/graphics/ImageObserver.h"
#include "platform/graphics/paint/SkPictureBuilder.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkColor.h"
#include "third_party/skia/include/core/SkPaint.h"
#include "skia/ext/cdl_picture.h"
#include "third_party/skia/include/core/SkRect.h"
#include "third_party/skia/include/core/SkSize.h"

namespace blink {

namespace {

// Gray with 40% opacity.
const RGBA32 kFillColor = 0x66808080;

}  // namespace

PlaceholderImage::~PlaceholderImage() {}

sk_sp<SkImage> PlaceholderImage::imageForCurrentFrame() {
  if (m_imageForCurrentFrame)
    return m_imageForCurrentFrame;

  const FloatRect destRect(0.0f, 0.0f, static_cast<float>(m_size.width()),
                           static_cast<float>(m_size.height()));
  SkPictureBuilder builder(destRect);
  GraphicsContext& context = builder.context();
  context.beginRecording(destRect);

  context.setFillColor(kFillColor);
  context.fillRect(destRect);

  m_imageForCurrentFrame = SkImage::MakeFromPicture(
      builder.endRecording()->toSkPicture(),
      SkISize::Make(m_size.width(), m_size.height()), nullptr, nullptr);

  return m_imageForCurrentFrame;
}

void PlaceholderImage::draw(CdlCanvas* canvas,
                            const CdlPaint& basePaint,
                            const FloatRect& destRect,
                            const FloatRect& srcRect,
                            RespectImageOrientationEnum,
                            ImageClampingMode) {
  if (!srcRect.intersects(FloatRect(0.0f, 0.0f,
                                    static_cast<float>(m_size.width()),
                                    static_cast<float>(m_size.height())))) {
    return;
  }

  CdlPaint paint(basePaint);
  paint.setStyle(CdlPaint::kFill_Style);
  paint.setColor(kFillColor);
  canvas->drawRect(destRect, paint);
}

void PlaceholderImage::destroyDecodedData() {
  m_imageForCurrentFrame.reset();
}

}  // namespace blink
