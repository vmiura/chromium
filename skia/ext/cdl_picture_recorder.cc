// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>
#include <stdint.h>

#include "third_party/skia/include/core/SkBBHFactory.h"
#include "skia/ext/cdl_lite_dl.h"
#include "skia/ext/cdl_picture_recorder.h"
#include "skia/ext/cdl_picture.h"

// TODO(cdl): Make recorder thread save.
std::shared_ptr<CdlLiteRecorder> CdlPictureRecorder::free_recorder;

CdlPictureRecorder::CdlPictureRecorder() {
  fActivelyRecording = false;
}
CdlPictureRecorder::~CdlPictureRecorder() {
  if (fRecorder.get())
    std::atomic_exchange(&free_recorder, fRecorder);
}

CdlCanvas* CdlPictureRecorder::beginRecording(const SkRect& bounds,
                                              SkBBHFactory* bbhFactory,
                                              uint32_t recordFlags) {
  fCullRect = bounds;
  fFlags = recordFlags;

  /*
  if (bbhFactory) {
    fBBH.reset((*bbhFactory)(cullRect));
    SkASSERT(fBBH.get());
  }
  */

  fRecord.reset(new CdlLiteDL(bounds));

#if 1
  if (!fRecorder.get())
    fRecorder = std::atomic_exchange(&free_recorder, fRecorder);

  if (fRecorder.get()) {
    fRecorder->reset(fRecord.get(), bounds);
  } else {
    fRecorder.reset(new CdlLiteRecorder(fRecord.get(), bounds));
  }
#else
  fRecorder.reset(new CdlLiteRecorder(fRecord.get(), bounds));
#endif

  fActivelyRecording = true;
  return this->getRecordingCanvas();
}

CdlCanvas* CdlPictureRecorder::getRecordingCanvas() {
  return fActivelyRecording ? fRecorder.get() : nullptr;
}

sk_sp<CdlPicture> CdlPictureRecorder::finishRecordingAsPicture(
    uint32_t endFlags) {
  fActivelyRecording = false;
  sk_sp<CdlPicture> pic = sk_make_sp<CdlPicture>(fRecord);
  return pic;
}
