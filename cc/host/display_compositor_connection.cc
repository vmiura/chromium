// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/host/display_compositor_connection.h"

#include "cc/host/display_compositor_connection_client.h"

namespace cc {

DisplayCompositorConnection::DisplayCompositorConnection(
    DisplayCompositorConnectionClient* connection_client,
    mojom::DisplayCompositorPtr display_compositor,
    mojom::DisplayCompositorClientRequest display_compositor_client)
    : connection_client_(connection_client),
      display_compositor_(std::move(display_compositor)),
      client_binding_(this, std::move(display_compositor_client)),
      weak_factory_(this) {
  display_compositor_.set_connection_error_handler(
      base::Bind(&DisplayCompositorConnection::OnConnectionLost,
                 weak_factory_.GetWeakPtr()));
}

DisplayCompositorConnection::~DisplayCompositorConnection() = default;

void DisplayCompositorConnection::RegisterContentFrameSinkObserver(
    const CompositorFrameSinkId& compositor_frame_sink_id,
    cc::mojom::ContentFrameSinkPrivateRequest private_request,
    cc::mojom::DisplayCompositorClientPtr display_compositor_client) {
  auto it = private_interfaces_.find(compositor_frame_sink_id);
  if (it != private_interfaces_.end()) {
    it->second->BindPrivilegedClient(std::move(private_request),
                                     std::move(display_compositor_client));
    return;
  }
  std::unique_ptr<ContentFrameSinkPrivate> content_frame_sink_private(
      new ContentFrameSinkPrivate(this, compositor_frame_sink_id,
                                  std::move(private_request),
                                  std::move(display_compositor_client)));
  private_interfaces_[compositor_frame_sink_id] =
      std::move(content_frame_sink_private);
}

void DisplayCompositorConnection::CreateContentFrameSink(
    uint32_t client_id,
    int32_t sink_id,
    const gpu::SurfaceHandle& handle,
    mojom::LayerTreeSettingsPtr settings,
    mojom::ContentFrameSinkRequest content_frame_sink,
    mojom::ContentFrameSinkClientPtr content_frame_sink_client) {
  auto it = private_interfaces_.find(CompositorFrameSinkId(client_id, sink_id));
  cc::mojom::ContentFrameSinkPrivateRequest private_request;
  if (it == private_interfaces_.end()) {
    CompositorFrameSinkId compositor_frame_sink_id(client_id, sink_id);
    cc::mojom::ContentFrameSinkPrivatePtr content_frame_sink_private_ptr;
    private_request = mojo::GetProxy(&content_frame_sink_private_ptr);
    std::unique_ptr<ContentFrameSinkPrivate> content_frame_sink_private(
        new ContentFrameSinkPrivate(this, compositor_frame_sink_id,
                                    std::move(content_frame_sink_private_ptr)));
    private_interfaces_[compositor_frame_sink_id] =
        std::move(content_frame_sink_private);
  } else {
    private_request = it->second->GetContentFrameSinkPrivateRequest();
  }
  display_compositor_->CreateContentFrameSink(
      client_id, sink_id, handle, std::move(settings),
      std::move(content_frame_sink), std::move(private_request),
      std::move(content_frame_sink_client));
}

void DisplayCompositorConnection::OnConnectionLost() {
  connection_client_->OnConnectionLost();
}

void DisplayCompositorConnection::OnPrivateConnectionLost(
    const CompositorFrameSinkId& compositor_frame_sink_id) {
  private_interfaces_.erase(compositor_frame_sink_id);
}

void DisplayCompositorConnection::OnSurfaceCreated(
    const gfx::Size& frame_size,
    const cc::SurfaceId& surface_id) {
  auto it = private_interfaces_.find(
      CompositorFrameSinkId(surface_id.client_id(), surface_id.sink_id()));
  if (it != private_interfaces_.end())
    it->second->OnSurfaceCreated(frame_size, surface_id);
}

}  // namespace cc
