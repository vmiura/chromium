// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/service/compositor_channel.h"

#include "cc/service/service.h"
#include "cc/service/service_factory.h"
#include "gpu/ipc/service/gpu_channel.h"

namespace cc {

CompositorChannel::CompositorChannel(ServiceFactory* factory,
                                     gpu::GpuChannel* channel)
    : factory_(factory), channel_(channel), binding_(this) {
  channel_->AddAssociatedInterface(
      base::Bind(&CompositorChannel::BindCompositorFactoryRequest,
                 base::Unretained(this)));
}

CompositorChannel::~CompositorChannel() = default;

void CompositorChannel::CreateCompositor(
    const gpu::SurfaceHandle& handle,
    cc::mojom::CompositorRequest compositor,
    cc::mojom::CompositorClientPtr compositor_client) {
  new Service(handle, std::move(compositor), std::move(compositor_client),
              factory_->NextServiceCompositorId(),
              factory_->shared_bitmap_manager(),
              factory_->gpu_memory_buffer_manager(), factory_->image_factory(),
              factory_->surface_manager(), factory_->task_graph_runner());
}

void CompositorChannel::BindCompositorFactoryRequest(
    cc::mojom::CompositorFactoryAssociatedRequest request) {
  binding_.Bind(std::move(request));
}

}  // namespace cc
