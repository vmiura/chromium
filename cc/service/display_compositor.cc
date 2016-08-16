// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/service/display_compositor.h"

#include "cc/service/service.h"
#include "cc/service/service_factory.h"
#include "cc/trees/layer_tree_settings.h"

namespace cc {

DisplayCompositor::DisplayCompositor(
    ServiceFactory* factory,
    mojom::DisplayCompositorRequest request,
    mojom::DisplayCompositorClientPtr client,
    scoped_refptr<base::SingleThreadTaskRunner> compositor_task_runner)
    : factory_(factory),
      compositor_task_runner_(compositor_task_runner),
      surface_manager_(this),
      client_(std::move(client)),
      display_compositor_binding_(this, std::move(request)) {
  task_graph_runner_.Start("CompositorWorker", base::SimpleThread::Options());
}

DisplayCompositor::~DisplayCompositor() = default;

void DisplayCompositor::RequestSurfaceManager(
    mojom::SurfaceManagerRequest compositor_channel) {
  surface_manager_bindings_.AddBinding(this, std::move(compositor_channel));
}

void DisplayCompositor::CreateCompositor(
    uint32_t client_id,
    const gpu::SurfaceHandle& handle,
    mojom::LayerTreeSettingsPtr settings,
    mojom::CompositorRequest compositor,
    mojom::CompositorClientPtr compositor_client) {
  LayerTreeSettings layer_tree_settings(settings.get());
  new Service(handle, std::move(compositor), std::move(compositor_client),
              layer_tree_settings,
              // TODO(fsamuel): Move to display compositor.
              next_service_id_++, factory_->shared_bitmap_manager(),
              factory_->gpu_memory_buffer_manager(),
              // This image factory is going to the wrong thread, but the
              // ServiceContextProvider will only use it on the main thread
              // thanks to our custom InProcessCommandBuffer::Service.
              factory_->image_factory(), &surface_manager_,
              &task_graph_runner_);
}

void DisplayCompositor::AddRefOnSurfaceId(const SurfaceId& id) {
  surface_manager_.AddRefOnSurfaceId(id);
}

void DisplayCompositor::MoveTempRefToRefOnSurfaceId(const SurfaceId& id) {
  surface_manager_.MoveTempRefToRefOnSurfaceId(id);
}

void DisplayCompositor::OnSurfaceCreated(const gfx::Size& size,
                                         const cc::SurfaceId& surface_id) {
  client_->OnSurfaceCreated(size, surface_id);
}

}  // namespace cc
