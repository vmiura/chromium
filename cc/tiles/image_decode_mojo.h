// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_TILES_IMAGE_DECODE_MOJO_H_
#define CC_TILES_IMAGE_DECODE_MOJO_H_

#include "cc/ipc/image_decode.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/bindings/interface_request.h"

namespace cc {

class ImageDecodeMojo : public cc::mojom::ImageDecode {
 public:
  ImageDecodeMojo(mojom::ImageDecodeRequest request);
  ~ImageDecodeMojo() override;

  // mojom::ImageDecode:
  void DecodeImage(uint32_t unique_id,
                   uint64_t data,
                   const DecodeImageCallback& callback) override;

 private:
  mojo::Binding<mojom::ImageDecode> binding_;
};

}  // namespace cc

#endif  // CC_TILES_IMAGE_DECODE_MOJO_H_
