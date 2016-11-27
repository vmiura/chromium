// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.


#include <stddef.h>
#include <stdint.h>

#include "cdl_picture_recorder.h"

#include "base/trace_event/trace_event.h"
#include "third_party/skia/include/core/SkBBHFactory.h"
#include "skia/ext/cdl_lite_dl.h"

#include "skia/ext/cdl_picture.h"

// TODO(cdl): Make recorder thread save.
std::shared_ptr<CdlLiteRecorder> CdlPictureRecorder::free_recorder;

CdlPictureRecorder::CdlPictureRecorder() {
  fActivelyRecording = false;
  start_offset_ = 0;
  fFlags = 0;
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

  //TRACE_EVENT_ASYNC_BEGIN0("cc", "CdlPictureRecorder::beginRecording", this);

  // Create new recorder only if it's above a threshold in size.
  if (!fRecord.get() || fRecord->getRecordOffset() >= 4096 - 256)
    fRecord.reset(new CdlLiteDL(bounds));
  else
    fRecord->resetForNextPicture(bounds);

  start_offset_ = fRecord->getRecordOffset();

  if (!fRecorder.get())
    fRecorder = std::atomic_exchange(&free_recorder, fRecorder);

  if (fRecorder.get()) {
    fRecorder->reset(fRecord.get(), bounds);
  } else {
    fRecorder.reset(new CdlLiteRecorder(fRecord.get(), bounds));
  }

  fActivelyRecording = true;
  return this->getRecordingCanvas();
}

CdlCanvas* CdlPictureRecorder::getRecordingCanvas() {
  return fActivelyRecording ? fRecorder.get() : nullptr;
}

sk_sp<CdlPicture> CdlPictureRecorder::finishRecordingAsPicture(
    uint32_t endFlags) {
  //TRACE_EVENT_ASYNC_END0("cc", "CdlPictureRecorder::beginRecording", this);

  fActivelyRecording = false;
  sk_sp<CdlPicture> pic = sk_make_sp<CdlPicture>(fRecord, fCullRect, start_offset_, fRecord->getRecordOffset());
  return pic;
}
