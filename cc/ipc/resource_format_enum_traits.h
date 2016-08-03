// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_IPC_RESOURCE_FORMAT_ENUM_TRAITS_H_
#define CC_IPC_RESOURCE_FORMAT_ENUM_TRAITS_H_

#include "cc/ipc/resource_format.mojom.h"
#include "cc/resources/resource_format.h"

namespace mojo {

template <>
struct EnumTraits<cc::mojom::ResourceFormat, cc::ResourceFormat> {
  static cc::mojom::ResourceFormat ToMojom(cc::ResourceFormat resource_format);
  static bool FromMojom(cc::mojom::ResourceFormat input,
                        cc::ResourceFormat* out);
};

}  // namespace mojo

#endif  // CC_IPC_RESOURCE_FORMAT_ENUM_TRAITS_H_
