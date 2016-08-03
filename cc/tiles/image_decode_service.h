// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_TILES_IMAGE_DECODE_SERVICE_H_
#define CC_TILES_IMAGE_DECODE_SERVICE_H_

#include "cc/base/cc_export.h"
#include "third_party/skia/include/core/SkImage.h"
#include "base/synchronization/lock.h"

#include <unordered_map>

namespace cc {

class CC_EXPORT ImageDecodeService {
 public:
  ImageDecodeService();
  ~ImageDecodeService();

  // TODO(hackathon): This shouldn't be a singleton.
  static ImageDecodeService* Current();

  // Blink accesses this to register things.
  void RegisterImage(sk_sp<SkImage> image);
  void UnregisterImage(sk_sp<SkImage> image);

  // Mojo accesses this?
  void DecodeImage(uint32_t image_id, void* buffer);

 private:
  void OnImageDecoded(uint32_t image_id, bool succeeded);

  base::Lock lock_;
  std::unordered_map<uint32_t, sk_sp<SkImage>> image_map_;

  DISALLOW_COPY_AND_ASSIGN(ImageDecodeService);
};

}  // namespace cc

#endif // CC_TILES_IMAGE_DECODE_SERVICE_H_
