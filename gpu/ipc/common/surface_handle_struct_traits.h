// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_IPC_COMMON_SURFACE_HANDLE_STRUCT_TRAITS_H_
#define GPU_IPC_COMMON_SURFACE_HANDLE_STRUCT_TRAITS_H_

#include "gpu/ipc/common/surface_handle.h"
#include "gpu/ipc/common/surface_handle.mojom.h"

namespace mojo {

template <>
struct StructTraits<gpu::mojom::SurfaceHandle, gpu::SurfaceHandle> {
  static gfx::AcceleratedWidget widget(const gpu::SurfaceHandle& handle) {
#if defined(GPU_SURFACE_HANDLE_IS_ACCELERATED_WINDOW)
    return handle;
#else
    return gfx::AcceleratedWidget();
#endif
  }

  static int32_t value(const gpu::SurfaceHandle& handle) {
#if !defined(GPU_SURFACE_HANDLE_IS_ACCELERATED_WINDOW)
    return handle;
#else
    return 0;
#endif
  }

  static bool Read(gpu::mojom::SurfaceHandleDataView data,
                   gpu::SurfaceHandle* handle) {
#if defined(GPU_SURFACE_HANDLE_IS_ACCELERATED_WINDOW)
    return data.ReadWidget(handle);
#else
    *handle = data.value();
    return true;
#endif
  }
};

}  // namespace mojo

#endif  // GPU_IPC_COMMON_SURFACE_HANDLE_STRUCT_TRAITS_H_
