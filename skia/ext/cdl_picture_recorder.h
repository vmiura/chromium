// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SKIA_EXT_CDL_PICTURE_RECORDER_H_
#define SKIA_EXT_CDL_PICTURE_RECORDER_H_

#include <stddef.h>
#include <stdint.h>

#include "skia/ext/cdl_lite_recorder.h"

#include "base/compiler_specific.h"
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
  uint32_t fFlags;
  SkRect fCullRect;

#ifdef SK_SUPPORT_LEGACY_CANVAS_IS_REFCNT
  sk_sp<CdlLiteRecorder> fRecorder;
#else
  std::unique_ptr<CdlLiteRecorder> fRecorder;
#endif

  sk_sp<CdlLiteDL> fRecord;

  // SkPictureRecorder picture_recorder_;
};

#endif  // SKIA_EXT_CDL_PICTURE_RECORDER_H_
