// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/service/display_compositor.h"

#include "cc/service/service.h"
#include "cc/service/service_factory.h"
#include "cc/trees/layer_tree_settings.h"

namespace cc {

namespace {

static void CreateServiceOnThread(
    const gpu::SurfaceHandle& handle,
    cc::mojom::LayerTreeSettingsPtr settings_mojom,
    cc::mojom::CompositorRequest compositor,
    cc::mojom::CompositorClientPtrInfo compositor_client_info,
    int id,
    SharedBitmapManager* shared_bitmap_manager,
    gpu::GpuMemoryBufferManager* gpu_mem,
    gpu::ImageFactory* image_factory,
    SurfaceManager* surface_manager,
    TaskGraphRunner* task_graph_runner) {
  LayerTreeSettings settings(settings_mojom.get());
  // HACKATHON: The service compositor uses this mode to give out a
  // BeginFrameSource, and the browser/renderer should have no control or input
  // on this decision anymore.
  settings.use_output_surface_begin_frame_source = true;
  DCHECK(!settings.renderer_settings.buffer_to_texture_target_map.empty());
  // Deletes itself.
  cc::mojom::CompositorClientPtr client_ptr;
  client_ptr.Bind(std::move(compositor_client_info));
  new Service(handle, std::move(compositor), std::move(client_ptr), settings,
              id, shared_bitmap_manager, gpu_mem, image_factory,
              surface_manager, task_graph_runner);
}

}  // namespace

DisplayCompositor::DisplayCompositor(
    ServiceFactory* factory,
    mojom::DisplayCompositorRequest request,
    mojom::DisplayCompositorClientPtr client,
    scoped_refptr<base::SingleThreadTaskRunner> compositor_task_runner)
    : factory_(factory),
      compositor_task_runner_(compositor_task_runner),
      client_(std::move(client)),
      binding_(this, std::move(request)) {}

DisplayCompositor::~DisplayCompositor() = default;

void DisplayCompositor::CreateCompositorChannel(
    mojom::CompositorChannelRequest compositor_channel) {
  // TODO(fsamuel): Figure out lifetime management.
  new CompositorChannel(std::move(compositor_channel), factory_,
                        compositor_task_runner_);
}

void DisplayCompositor::CreateCompositor(
    uint32_t client_id,
    const gpu::SurfaceHandle& handle,
    mojom::LayerTreeSettingsPtr settings,
    mojom::CompositorRequest compositor,
    mojom::CompositorClientPtr compositor_client) {
  compositor_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&CreateServiceOnThread, handle, base::Passed(&settings),
                 base::Passed(&compositor),
                 base::Passed(compositor_client.PassInterface()),
                 // client_id,
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
                 factory_->surface_manager(), factory_->task_graph_runner()));
}

}  // namespace cc
