// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/test/picture_cache_model.h"

#include "base/logging.h"
#include "third_party/skia/include/core/SkData.h"
#include "skia/ext/cdl_picture.h"
#include "third_party/skia/include/core/SkRefCnt.h"

class SkPixelSerializer;

namespace cc {
namespace {

/*
sk_sp<SkPicture> CopySkPicture(const SkPicture* picture) {
  sk_sp<SkData> data = picture->serialize();
  DCHECK_GT(data->size(), 0u);
  return SkPicture::MakeFromData(data.get());
}
*/

}  // namespace

PictureCacheModel::PictureCacheModel() = default;

PictureCacheModel::~PictureCacheModel() = default;

void PictureCacheModel::AddPicture(const CdlPicture* picture) {
  // sk_sp<CdlPicture> picture_copy = CopySkPicture(picture);
  // TODO(cdl): CopyCdlPicture
  sk_sp<const CdlPicture> picture_copy = sk_ref_sp(picture);
  pictures_.insert(std::make_pair(picture->uniqueID(), picture_copy));
}

sk_sp<const CdlPicture> PictureCacheModel::GetPicture(uint32_t unique_id) {
  if (pictures_.find(unique_id) == pictures_.end())
    return nullptr;

  return pictures_.find(unique_id)->second;
}

}  // namespace cc
