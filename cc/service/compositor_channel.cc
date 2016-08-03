// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/service/compositor_channel.h"

#include "cc/base/completion_event.h"
#include "cc/service/service.h"
#include "cc/service/service_factory.h"
#include "gpu/ipc/service/gpu_channel.h"

namespace cc {

CompositorChannel::CompositorChannel(
    ServiceFactory* factory,
    gpu::GpuChannel* channel,
    scoped_refptr<base::SingleThreadTaskRunner> compositor_thread)
    : factory_(factory), channel_(channel) {
  channel_->AddAssociatedInterface(
      base::Bind(&CompositorChannel::BindCompositorFactoryRequest,
                 base::Unretained(this), base::Passed(&compositor_thread)));
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

void CompositorChannel::BindOnCompositorThread(
    cc::mojom::CompositorFactoryAssociatedRequest* request,
    CompletionEvent* event) {
  binding_.reset(new CompositorBinding(this));
  binding_->Bind(std::move(*request));
  event->Signal();
}

void CompositorChannel::BindCompositorFactoryRequest(
    scoped_refptr<base::SingleThreadTaskRunner> compositor_thread,
    cc::mojom::CompositorFactoryAssociatedRequest request) {
  // Bind should happen on the thread we want to receive messages on.
  CompletionEvent event;
  compositor_thread->PostTask(
      FROM_HERE, base::Bind(&CompositorChannel::BindOnCompositorThread,
                            base::Unretained(this), &request, &event));
  event.Wait();
}

}  // namespace cc
