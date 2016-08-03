// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/ipc/renderer_capabilities_struct_traits.h"

#include "cc/ipc/resource_format_enum_traits.h"

namespace mojo {

// static
bool StructTraits<cc::mojom::RendererCapabilities, cc::RendererCapabilities>::
    Read(cc::mojom::RendererCapabilitiesDataView data,
         cc::RendererCapabilities* out) {
  if (!data.ReadBestTextureFormat(&out->best_texture_format))
    return false;

  out->allow_partial_texture_updates = data.allow_partial_texture_updates();
  out->max_texture_size = data.max_texture_size();
  out->using_shared_memory_resources = data.using_shared_memory_resources();
  return true;
}

}  // namespace mojo
