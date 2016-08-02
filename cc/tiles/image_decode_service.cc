// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/tiles/image_decode_service.h"
#include "cc/resources/resource_format_utils.h"

namespace cc {
namespace {
SkImageInfo CreateImageInfo(size_t width,
                            size_t height,
                            ResourceFormat format) {
  return SkImageInfo::Make(width, height,
                           ResourceFormatToClosestSkColorType(format),
                           kPremul_SkAlphaType);
}
}  // namespace

ImageDecodeService::ImageDecodeService() = default;
ImageDecodeService::~ImageDecodeService() = default;

void ImageDecodeService::RegisterImage(sk_sp<SkImage> image) {
  base::AutoLock hold(lock_);
  image_map_[image->uniqueID()] = std::move(image);
}

void ImageDecodeService::UnregisterImage(sk_sp<SkImage> image) {
  base::AutoLock hold(lock_);
  image_map_.erase(image->uniqueID());
}

void ImageDecodeService::DecodeImage(uint32_t image_id, void* buffer) {
  // TODO(hackathon): Thread me plz.
  sk_sp<SkImage> image = [this, image_id]() {
    base::AutoLock hold(lock_);
    DCHECK(image_map_.find(image_id) != image_map_.end());
    return image_map_[image_id];
  }();

  // TODO(hackathon): Change format.
  SkImageInfo decoded_info =
      CreateImageInfo(image->width(), image->height(), RGBA_8888);

  bool result =
      image->readPixels(decoded_info, buffer, decoded_info.minRowBytes(), 0, 0,
                        SkImage::kDisallow_CachingHint);
  OnImageDecoded(image_id, result);
}

void ImageDecodeService::OnImageDecoded(uint32_t image_id, bool succeeded) {
  // TODO(hackathon): Send IPC.
}

}  // namespace cc
