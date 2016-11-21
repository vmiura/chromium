// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/graphics/PaintGeneratedImage.h"

#include "platform/geometry/FloatRect.h"
#include "platform/graphics/GraphicsContext.h"
#include "skia/ext/cdl_picture.h"

namespace blink {

void PaintGeneratedImage::draw(SkCanvas* canvas,
                               const SkPaint& paint,
                               const FloatRect& destRect,
                               const FloatRect& srcRect,
                               RespectImageOrientationEnum,
                               ImageClampingMode) {
  SkAutoCanvasRestore ar(canvas, true);
  canvas->clipRect(destRect);
  canvas->translate(destRect.x(), destRect.y());
  if (destRect.size() != srcRect.size())
    canvas->scale(destRect.width() / srcRect.width(),
                  destRect.height() / srcRect.height());
  canvas->translate(-srcRect.x(), -srcRect.y());
  // TODO(cdl): Need SkPaint on drawDrawable?
  // canvas->drawDrawable(m_picture.get(), nullptr, &paint);
  m_picture->draw(canvas);
}

void PaintGeneratedImage::drawTile(GraphicsContext& context,
                                   const FloatRect& srcRect) {
  context.drawPicture(m_picture.get());
}

}  // namespace blink
