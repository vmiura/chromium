// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/service/compositor_channel.h"

#include "cc/service/service.h"
#include "cc/service/service_factory.h"
#include "gpu/ipc/service/gpu_channel.h"

namespace cc {

CompositorChannel::CompositorChannel(
    ServiceFactory* factory,
    gpu::GpuChannel* channel,
    scoped_refptr<base::SingleThreadTaskRunner> compositor_task_runner)
    : factory_(factory), channel_(channel), binding_(this), compositor_task_runner_(compositor_task_runner) {
  channel_->AddAssociatedInterface(
      base::Bind(&CompositorChannel::BindCompositorFactoryRequest,
                 base::Unretained(this)));
}

CompositorChannel::~CompositorChannel() = default;

static void CreateServiceOnThread(
    const gpu::SurfaceHandle& handle,
    cc::mojom::CompositorRequest compositor,
    cc::mojom::CompositorClientPtrInfo compositor_client_info,
    int id, SharedBitmapManager* shared_bitmap_manager,
    gpu::GpuMemoryBufferManager* gpu_mem,
    gpu::ImageFactory* image_factory,
    SurfaceManager* surface_manager,
    TaskGraphRunner* task_graph_runner) {
  // Deletes itself.
  cc::mojom::CompositorClientPtr client_ptr;
  client_ptr.Bind(std::move(compositor_client_info));
  new Service(handle, std::move(compositor),
              std::move(client_ptr),
              id, shared_bitmap_manager, gpu_mem,
              image_factory,
              surface_manager,
              task_graph_runner);
}

void CompositorChannel::CreateCompositor(
    const gpu::SurfaceHandle& handle,
    cc::mojom::CompositorRequest compositor,
    cc::mojom::CompositorClientPtr compositor_client) {
  LOG(ERROR) << "CreateCompositor";
  compositor_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&CreateServiceOnThread, handle, base::Passed(&compositor),
                 base::Passed(compositor_client.PassInterface()),
                 factory_->NextServiceCompositorId(),
                 // TODO(hackathon): Better be threadsafe.
                 factory_->shared_bitmap_manager(),
                 // TODO(hackathon): Better be threadsafe.
                 factory_->gpu_memory_buffer_manager(),
                 // This image factory is going to the wrong thread, but the
                 // ServiceContextProvider will only use it on the main thread
                 // thanks to our custom InProcessCommandBuffer::Service.
                 factory_->image_factory(),
                 // These two are owned on the main thread, but will only be
                 // used on the compositor thread which is joined before they
                 // are destroyed.
                 factory_->surface_manager(),
                 factory_->task_graph_runner()));
}

void CompositorChannel::BindCompositorFactoryRequest(
    cc::mojom::CompositorFactoryAssociatedRequest request) {
  binding_.Bind(std::move(request));
}

}  // namespace cc
