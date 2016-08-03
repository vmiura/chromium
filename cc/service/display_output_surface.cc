// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/service/display_output_surface.h"

#include <utility>

#include "base/auto_reset.h"
#include "cc/output/compositor_frame.h"
#include "cc/output/output_surface_client.h"
#include "cc/service/service_context_provider.h"
#include "gpu/command_buffer/client/context_support.h"
#include "gpu/command_buffer/client/gles2_interface.h"
#include "third_party/khronos/GLES2/gl2.h"
#include "ui/gfx/transform.h"

namespace cc {

DisplayOutputSurface::DisplayOutputSurface(
    scoped_refptr<ServiceContextProvider> context_provider)
    : OutputSurface(std::move(context_provider), nullptr, nullptr) {
  capabilities_.adjust_deadline_for_parent = false;
  // TODO(hackathon): Get real value (from context provider?)
  capabilities_.flipped_output_surface = false;
}

DisplayOutputSurface::~DisplayOutputSurface() = default;

void DisplayOutputSurface::SwapBuffers(CompositorFrame frame) {
  DCHECK(frame.gl_frame_data);

  auto* gl = context_provider_->ContextGL();
  auto* support = context_provider_->ContextSupport();

  gfx::Rect swap_rect(frame.gl_frame_data->sub_buffer_rect);
  gfx::Rect frame_rect(frame.gl_frame_data->size);
  if (swap_rect == frame_rect)
    support->Swap();
  else
    support->PartialSwapBuffers(swap_rect);
  // TODO(hackathon): Needs to actually get back a swap complete signal from the
  // command buffer and not flush here.
  gl->Flush();
  PostSwapBuffersComplete();
}

uint32_t DisplayOutputSurface::GetFramebufferCopyTextureFormat() {
  return static_cast<ServiceContextProvider*>(context_provider())
      ->GetCopyTextureInternalFormat();
}

}  // namespace cc
