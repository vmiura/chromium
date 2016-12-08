// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>
#include <stdint.h>

#include "cdl_picture_recorder.h"

#include "base/trace_event/trace_event.h"
#include "third_party/skia/include/core/SkBBHFactory.h"
#include "skia/ext/cdl_picture_buffer.h"

#include "skia/ext/cdl_picture.h"
#include "skia/ext/cdl_picture_recording_canvas.h"

base::Lock CdlPictureRecorder::lock;
std::shared_ptr<CdlPictureRecordingCanvas> CdlPictureRecorder::free_recorder;

CdlPictureRecorder::CdlPictureRecorder() {
  fActivelyRecording = false;
  start_offset_ = 0;
  fFlags = 0;
}
CdlPictureRecorder::~CdlPictureRecorder() {
  if (fRecorder.get()) {
    base::AutoLock hold(lock);
    fRecorder.swap(free_recorder);
  }
}

CdlCanvas* CdlPictureRecorder::beginRecording(const SkRect& bounds,
                                              SkBBHFactory* bbhFactory,
                                              uint32_t recordFlags) {
  fCullRect = bounds;
  fFlags = recordFlags;

  // TODO(cdl): Painting is relying on a funky behavior of SkPictureRecorder;
  // calling beginRecording multiple times results in appending to the
  // existing SkRecord, but resetting the SkRecorder which seems wrong.
  if (fActivelyRecording)
    return fRecorder.get();

  /*
  if (bbhFactory) {
    fBBH.reset((*bbhFactory)(cullRect));
    SkASSERT(fBBH.get());
  }
  */

  // TRACE_EVENT_ASYNC_BEGIN0("cc", "CdlPictureRecorder::beginRecording", this);

  // Create new recorder only if it's above a threshold in size.
  if (!fRecord.get() || fRecord->getRecordOffset() >= 4096 - 256)
    fRecord.reset(new CdlPictureBuffer(bounds));
  else
    fRecord->resetForNextPicture(bounds);

  start_offset_ = fRecord->getRecordOffset();

  if (!fRecorder.get()) {
    base::AutoLock hold(lock);
    fRecorder.swap(free_recorder);
  }

  if (fRecorder.get()) {
    fRecorder->reset(fRecord.get(), bounds);
  } else {
    fRecorder.reset(new CdlPictureRecordingCanvas(fRecord.get(), bounds));
  }

  fActivelyRecording = true;
  return this->getRecordingCanvas();
}

CdlCanvas* CdlPictureRecorder::getRecordingCanvas() {
  return fActivelyRecording ? fRecorder.get() : nullptr;
}

sk_sp<CdlPicture> CdlPictureRecorder::finishRecordingAsPicture(
    uint32_t endFlags) {
  // TRACE_EVENT_ASYNC_END0("cc", "CdlPictureRecorder::beginRecording", this);
  //CHECK(fActivelyRecording);
  fActivelyRecording = false;

  fRecorder->restoreToCount(1);  // If we were missing any restores, add them now.

  sk_sp<CdlPicture> pic = sk_make_sp<CdlPicture>(
      fRecord, fCullRect, start_offset_, fRecord->getRecordOffset());
  return pic;
}
