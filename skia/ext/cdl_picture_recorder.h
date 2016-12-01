// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SKIA_EXT_CDL_PICTURE_RECORDER_H_
#define SKIA_EXT_CDL_PICTURE_RECORDER_H_

#include <stddef.h>
#include <stdint.h>

#include "skia/ext/cdl_lite_recorder.h"

#include "base/compiler_specific.h"
#include "base/synchronization/lock.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkBBHFactory.h"
#include "third_party/skia/include/core/SkPictureRecorder.h"

class CdlLiteDL;
class CdlLiteRecorder;
class CdlCanvas;
class CdlPicture;

class CdlPictureRecorder : SkNoncopyable {
 public:
  CdlPictureRecorder();
  ~CdlPictureRecorder();

  CdlCanvas* beginRecording(const SkRect& bounds,
                            SkBBHFactory* bbhFactory = NULL,
                            uint32_t recordFlags = 0);

  CdlCanvas* beginRecording(SkScalar width,
                            SkScalar height,
                            SkBBHFactory* bbhFactory = NULL,
                            uint32_t recordFlags = 0) {
    return this->beginRecording(SkRect::MakeWH(width, height), bbhFactory,
                                recordFlags);
  }

  /** Returns the recording canvas if one is active, or NULL if recording is
      not active. This does not alter the refcnt on the canvas (if present).
  */
  CdlCanvas* getRecordingCanvas();

  sk_sp<CdlPicture> finishRecordingAsPicture(uint32_t endFlags = 0);

 private:
  void reset();

  bool fActivelyRecording;
  int start_offset_;
  uint32_t fFlags;
  SkRect fCullRect;

  std::shared_ptr<CdlLiteRecorder> fRecorder;
  sk_sp<CdlLiteDL> fRecord;

  static base::Lock lock;
  static std::shared_ptr<CdlLiteRecorder> free_recorder;

  // SkPictureRecorder picture_recorder_;
};

#endif  // SKIA_EXT_CDL_PICTURE_RECORDER_H_
