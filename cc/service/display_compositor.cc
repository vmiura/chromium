// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/service/display_compositor.h"

#include "cc/service/content_frame_sink.h"
#include "cc/service/display_compositor_factory.h"
#include "cc/trees/layer_tree_settings.h"

namespace cc {

DisplayCompositor::DisplayCompositor(DisplayCompositorFactory* factory,
                                     mojom::DisplayCompositorRequest request,
                                     mojom::DisplayCompositorClientPtr client)
    : factory_(factory),
      surface_manager_(this),
      client_(std::move(client)),
      display_compositor_binding_(this, std::move(request)) {
  task_graph_runner_.Start("CompositorWorker", base::SimpleThread::Options());
}

DisplayCompositor::~DisplayCompositor() = default;

void DisplayCompositor::CreateContentFrameSink(
    uint32_t client_id,
    int32_t sink_id,
    const gpu::SurfaceHandle& handle,
    mojom::LayerTreeSettingsPtr settings,
    mojom::ContentFrameSinkRequest content_frame_sink,
    mojom::ContentFrameSinkPrivateRequest content_frame_sink_private,
    mojom::ContentFrameSinkClientPtr content_frame_sink_client) {
  LayerTreeSettings layer_tree_settings(settings.get());
  content_frame_sinks_[CompositorFrameSinkId(client_id, sink_id)] =
      base::MakeUnique<ContentFrameSink>(
          this, client_id, sink_id, handle, std::move(content_frame_sink),
          std::move(content_frame_sink_private),
          std::move(content_frame_sink_client), layer_tree_settings,
          factory_->shared_bitmap_manager(),
          factory_->gpu_memory_buffer_manager(),
          // This image factory is going to the wrong thread, but the
          // ServiceContextProvider will only use it on the main thread
          // thanks to our custom InProcessCommandBuffer::Service.
          factory_->image_factory(), &surface_manager_, &task_graph_runner_);
}

void DisplayCompositor::OnSurfaceCreated(const gfx::Size& size,
                                         const cc::SurfaceId& surface_id) {
  client_->OnSurfaceCreated(size, surface_id);
}

void DisplayCompositor::OnContentFrameSinkClientConnectionLost(
    const cc::CompositorFrameSinkId& compositor_frame_sink_id,
    bool destroy_content_frame_sink) {
  if (destroy_content_frame_sink)
    content_frame_sinks_.erase(compositor_frame_sink_id);
  client_->OnContentFrameSinkClientConnectionLost(compositor_frame_sink_id);
}

void DisplayCompositor::OnContentFrameSinkPrivateConnectionLost(
    const cc::CompositorFrameSinkId& compositor_frame_sink_id,
    bool destroy_content_frame_sink) {
  if (destroy_content_frame_sink)
    content_frame_sinks_.erase(compositor_frame_sink_id);
}

}  // namespace cc
