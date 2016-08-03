// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_IPC_RENDERER_CAPABILITIES_STRUCT_TRAITS_H_
#define CC_IPC_RENDERER_CAPABILITIES_STRUCT_TRAITS_H_

#include "cc/output/renderer_capabilities.h"
#include "cc/ipc/renderer_capabilities.mojom.h"

namespace mojo {

template <>
struct StructTraits<cc::mojom::RendererCapabilities, cc::RendererCapabilities> {
  static cc::ResourceFormat best_texture_format(
      const cc::RendererCapabilities& capabilities) {
    return capabilities.best_texture_format;
  }

  static bool allow_partial_texture_updates(
      const cc::RendererCapabilities& capabilities) {
    return capabilities.allow_partial_texture_updates;
  }

  static int32_t max_texture_size(
      const cc::RendererCapabilities& capabilities) {
    return capabilities.max_texture_size;
  }

  static bool using_shared_memory_resources(
      const cc::RendererCapabilities& capabilities) {
    return capabilities.using_shared_memory_resources;
  }

  static bool Read(cc::mojom::RendererCapabilitiesDataView data,
                   cc::RendererCapabilities* out);
};

}  // namespace mojo

#endif  // CC_IPC_RENDERER_CAPABILITIES_STRUCT_TRAITS_H_
