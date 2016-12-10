// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "skia/ext/cdl_picture.h"

#if CDL_ENABLED

#include <stddef.h>
#include <stdint.h>

#include "base/atomicops.h"
#include "base/logging.h"
#include "third_party/skia/include/core/SkPicture.h"
#include "third_party/skia/include/core/SkPictureRecorder.h"

#include "skia/ext/cdl_picture_buffer.h"

CdlPicture::CdlPicture(sk_sp<CdlPictureBuffer> picture,
                       SkRect cull_bounds,
                       int start_offset,
                       int end_offset)
    : picture_(picture),
      cull_bounds_(cull_bounds),
      start_offset_(start_offset),
      end_offset_(end_offset) {}

CdlPicture::~CdlPicture() {}

void CdlPicture::draw(CdlCanvas* canvas,
                      const SkMatrix* matrix,
                      const CdlPaint* paint) const {
  canvas->drawPicture(this, matrix, paint);
}

sk_sp<SkPicture> CdlPicture::toSkPicture() const {
  SkPictureRecorder recorder;
  SkCanvas* canvas = recorder.beginRecording(cullRect());
  picture_->playback(CdlCanvas::Make(canvas).get(), start_offset_, end_offset_);

  return recorder.finishRecordingAsPicture();
}

void CdlPicture::playback(CdlCanvas* canvas,
                          SkPicture::AbortCallback* callback) const {
  // TODO(cdl): CdlPicture doesn't support AbortCallback.
  int save_count = canvas->getSaveCount();
  canvas->save();
  picture_->playback(canvas, start_offset_, end_offset_);
  canvas->restoreToCount(save_count);
}

uint32_t CdlPicture::uniqueID() const {
  if (!unique_id_) {
    static base::subtle::Atomic32 g_next_id = 1;
    unique_id_ = base::subtle::Barrier_AtomicIncrement(&g_next_id, 1);
  }
  return unique_id_;
}

#endif  // CDL_ENABLED
