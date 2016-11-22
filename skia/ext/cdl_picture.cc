// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>
#include <stdint.h>

#include "base/logging.h"
#include "third_party/skia/include/core/SkPicture.h"
#include "third_party/skia/include/core/SkPictureRecorder.h"
#include "skia/ext/cdl_picture.h"
#include "skia/ext/cdl_lite_dl.h"

CdlPicture::CdlPicture(sk_sp<CdlLiteDL> picture) : picture_(picture) {}

CdlPicture::~CdlPicture() {}

void CdlPicture::draw(SkCanvas* canvas) const {
  canvas->drawDrawable(picture_.get());
}

sk_sp<SkPicture> CdlPicture::toSkPicture() const {
  SkPictureRecorder recorder;
  SkRect bounds = picture_->getBounds();
  SkCanvas* canvas = recorder.beginRecording(bounds);
  canvas->drawDrawable(picture_.get());
  return recorder.finishRecordingAsPicture();
}

sk_sp<SkDrawable> CdlPicture::toSkDrawable() const {
  return nullptr;
}

void CdlPicture::playback(SkCanvas* canvas,
                          SkPicture::AbortCallback* callback) const {
  // TODO(cdl): SkDrawable doesn't support AbortCallback.
  canvas->drawDrawable(picture_.get());
}

SkRect CdlPicture::cullRect() const {
  return picture_->getBounds();
}

uint32_t CdlPicture::uniqueID() const {
  return picture_->getGenerationID();
}
