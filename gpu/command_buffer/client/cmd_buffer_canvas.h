// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_COMMAND_BUFFER_CLIENT_CMD_BUFFER_CANVAS_H_
#define GPU_COMMAND_BUFFER_CLIENT_CMD_BUFFER_CANVAS_H_

#include "gpu/gpu_export.h"

class SkCanvas;

namespace gpu {

class ContextSupport;

namespace gles2 {
class GLES2Interface;
}  // namepace gles2

GPU_EXPORT SkCanvas* MakeCommandBufferCanvas(
    int width,
    int height,
    gpu::gles2::GLES2Interface* gl,
    gpu::ContextSupport* context_support);

}  // namepace gpu

#endif  // GPU_COMMAND_BUFFER_CLIENT_CMD_BUFFER_CANVAS_H_
