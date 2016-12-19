// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SKIA_EXT_CDL_PICTURE_H_
#define SKIA_EXT_CDL_PICTURE_H_

#include "cdl_common.h"

#if CDL_ENABLED

#include <stddef.h>
#include <stdint.h>

#include "base/compiler_specific.h"
#include "cdl_canvas.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkPicture.h"
#include "third_party/skia/include/core/SkDrawable.h"

class CdlPictureBuffer;

class SK_API CdlPicture : public SkRefCnt {
 public:
  CdlPicture(sk_sp<CdlPictureBuffer> dl,
             SkRect cull_bounds,
             int start_offset,
             int end_offset);
  ~CdlPicture() override;

  void draw(CdlCanvas* canvas,
            const SkMatrix* matrix,
            const CdlPaint* paint) const;

  sk_sp<SkPicture> toSkPicture() const;
  int approximateOpCount() const { return 1; }
  void playback(CdlCanvas*, SkPicture::AbortCallback* = NULL) const;
  SkRect cullRect() const { return cull_bounds_; }
  uint32_t uniqueID() const;

 private:
  sk_sp<CdlPictureBuffer> picture_;
  SkRect cull_bounds_;
  int start_offset_;
  int end_offset_;
  mutable uint32_t unique_id_ = 0;
};

inline sk_sp<SkPicture> ToSkPicture(CdlPicture* picture) {
  return picture->toSkPicture();
}

inline sk_sp<const SkPicture> ToSkPicture(const CdlPicture* picture) {
  return picture->toSkPicture();
}

#else

#include "third_party/skia/include/core/SkPicture.h"

inline sk_sp<SkPicture> ToSkPicture(CdlPicture* picture) {
  return sk_sp<SkPicture>(picture);
}

inline sk_sp<const SkPicture> ToSkPicture(const CdlPicture* picture) {
  return sk_sp<const SkPicture>(picture);
}

#endif  // CDL_ENABLED

#endif  // SKIA_EXT_CDL_PICTURE_H_
