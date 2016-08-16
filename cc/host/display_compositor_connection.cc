// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/host/display_compositor_connection.h"

namespace cc {

DisplayCompositorConnection::DisplayCompositorConnection(
    mojom::DisplayCompositorPtr display_compositor,
    mojom::DisplayCompositorClientRequest display_compositor_client)
    : display_compositor_(std::move(display_compositor)),
      client_binding_(this, std::move(display_compositor_client)) {}

DisplayCompositorConnection::~DisplayCompositorConnection() = default;

void DisplayCompositorConnection::RequestSurfaceManager(
    mojom::SurfaceManagerRequest surface_manager) {
  display_compositor_->RequestSurfaceManager(std::move(surface_manager));
}

void DisplayCompositorConnection::CreateContentFrameSink(
    uint32_t client_id,
    const gpu::SurfaceHandle& handle,
    mojom::LayerTreeSettingsPtr settings,
    mojom::ContentFrameSinkRequest content_frame_sink,
    mojom::ContentFrameSinkClientPtr content_frame_sink_client) {
  display_compositor_->CreateContentFrameSink(
      client_id, handle, std::move(settings), std::move(content_frame_sink),
      std::move(content_frame_sink_client));
}

void DisplayCompositorConnection::OnSurfaceCreated(
    const gfx::Size& frame_size,
    const cc::SurfaceId& surface_id) {
  fprintf(stderr, ">>>>OnSurfaceCreated frame_size(%d, %d) surface_id: %s\n",
          frame_size.width(), frame_size.height(),
          surface_id.ToString().c_str());
}

}  // namespace cc
