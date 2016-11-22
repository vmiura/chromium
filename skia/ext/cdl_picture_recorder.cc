// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>
#include <stdint.h>

#include "third_party/skia/include/core/SkBBHFactory.h"
#include "skia/ext/cdl_lite_dl.h"
#include "skia/ext/cdl_picture_recorder.h"
#include "skia/ext/cdl_picture.h"

CdlPictureRecorder::CdlPictureRecorder() {
  fActivelyRecording = false;
}
CdlPictureRecorder::~CdlPictureRecorder() {}

CdlCanvas* CdlPictureRecorder::beginRecording(const SkRect& bounds,
                                             SkBBHFactory* bbhFactory,
                                             uint32_t recordFlags) {
  // return picture_recorder_.beginRecording(bounds, bbhFactory, recordFlags);

  fCullRect = bounds;
  fFlags = recordFlags;

  /*
  if (bbhFactory) {
    fBBH.reset((*bbhFactory)(cullRect));
    SkASSERT(fBBH.get());
  }
  */

  fRecord.reset(new CdlLiteDL(bounds));
  fRecorder.reset(new CdlLiteRecorder(fRecord.get(), bounds));

  fActivelyRecording = true;
  return this->getRecordingCanvas();
}

CdlCanvas* CdlPictureRecorder::getRecordingCanvas() {
  // return picture_recorder_.getRecordingCanvas();
  return fRecorder.get();
}

sk_sp<CdlPicture> CdlPictureRecorder::finishRecordingAsPicture(
    uint32_t endFlags) {
  sk_sp<CdlPicture> pic = sk_make_sp<CdlPicture>(fRecord);
  return pic;
}
