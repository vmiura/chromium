// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/tiles/image_decode_mojo.h"

#include "base/trace_event/trace_event.h"
#include "cc/tiles/image_decode_service.h"

namespace cc {

ImageDecodeMojo::ImageDecodeMojo(mojom::ImageDecodeRequest request)
    : binding_(this, std::move(request)) {
  VLOG(0) << "ImageDecodeMojo is bound";
}
ImageDecodeMojo::~ImageDecodeMojo() {}

void ImageDecodeMojo::DecodeImage(uint32_t unique_id,
                                  uint64_t data,
                                  const DecodeImageCallback& callback) {
  TRACE_EVENT1("cc", "ImageDecodeMojo::DecodeImage", "unique_id", unique_id);

  void* data_ptr = (void*)data;
  VLOG(0) << "ImageDecodeMojo::DecodeImage " << unique_id << " " << data_ptr;

  ImageDecodeService::Current()->DecodeImage(unique_id, data_ptr, callback);
}

}  // namespace cc
