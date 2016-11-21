// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SKIA_EXT_CDL_PICTURE_H_
#define SKIA_EXT_CDL_PICTURE_H_

#include <stddef.h>
#include <stdint.h>

#include "cdl_lite_dl.h"

#include "base/compiler_specific.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkPicture.h"
#include "third_party/skia/include/core/SkDrawable.h"

class CdlPicture : public SkRefCnt {
 public:
  CdlPicture(sk_sp<CdlLiteDL> picture);
  ~CdlPicture() override;

  void draw(SkCanvas* canvas) const;

  sk_sp<SkPicture> toSkPicture() const;
  sk_sp<SkDrawable> toSkDrawable() const;
  int approximateOpCount() const { return 1; }
  void playback(SkCanvas*, SkPicture::AbortCallback* = NULL) const;
  SkRect cullRect() const;
  uint32_t uniqueID() const;

 private:
  sk_sp<CdlLiteDL> picture_;
};

#endif  // SKIA_EXT_CDL_PICTURE_H_
