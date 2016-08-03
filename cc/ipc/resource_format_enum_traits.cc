// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/ipc/resource_format_enum_traits.h"

namespace mojo {

// static
cc::mojom::ResourceFormat
EnumTraits<cc::mojom::ResourceFormat, cc::ResourceFormat>::ToMojom(
    cc::ResourceFormat resource_format) {
  switch (resource_format) {
    case cc::RGBA_8888:
      return cc::mojom::ResourceFormat::RGBA_8888;
    case cc::RGBA_4444:
      return cc::mojom::ResourceFormat::RGBA_4444;
    case cc::BGRA_8888:
      return cc::mojom::ResourceFormat::BGRA_8888;
    case cc::ALPHA_8:
      return cc::mojom::ResourceFormat::ALPHA_8;
    case cc::LUMINANCE_8:
      return cc::mojom::ResourceFormat::LUMINANCE_8;
    case cc::RGB_565:
      return cc::mojom::ResourceFormat::RGB_565;
    case cc::ETC1:
      return cc::mojom::ResourceFormat::ETC1;
    case cc::RED_8:
      return cc::mojom::ResourceFormat::RED_8;
    case cc::LUMINANCE_F16:
      return cc::mojom::ResourceFormat::LUMINANCE_F16;
  }
  NOTREACHED();
  return cc::mojom::ResourceFormat::RGBA_8888;
}

// static
bool EnumTraits<cc::mojom::ResourceFormat, cc::ResourceFormat>::FromMojom(
    cc::mojom::ResourceFormat input,
    cc::ResourceFormat* out) {
  switch (input) {
    case cc::mojom::ResourceFormat::RGBA_8888:
      *out = cc::RGBA_8888;
      return true;
    case cc::mojom::ResourceFormat::RGBA_4444:
      *out = cc::RGBA_4444;
      return true;
    case cc::mojom::ResourceFormat::BGRA_8888:
      *out = cc::BGRA_8888;
      return true;
    case cc::mojom::ResourceFormat::ALPHA_8:
      *out = cc::ALPHA_8;
      return true;
    case cc::mojom::ResourceFormat::LUMINANCE_8:
      *out = cc::LUMINANCE_8;
      return true;
    case cc::mojom::ResourceFormat::RGB_565:
      *out = cc::RGB_565;
      return true;
    case cc::mojom::ResourceFormat::ETC1:
      *out = cc::ETC1;
      return true;
    case cc::mojom::ResourceFormat::RED_8:
      *out = cc::RED_8;
      return true;
    case cc::mojom::ResourceFormat::LUMINANCE_F16:
      *out = cc::LUMINANCE_F16;
      return true;
  }
  NOTREACHED();
  return cc::RGBA_8888;
}

}  // namespace mojo
