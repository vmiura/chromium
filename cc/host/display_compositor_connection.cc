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
    cc::mojom::ContentFrameSinkObserverPtr content_frame_sink_observer) {
  auto it = private_interfaces_.find(compositor_frame_sink_id);
  if (it != private_interfaces_.end()) {
    it->second->BindPrivilegedClient(std::move(private_request),
                                     std::move(content_frame_sink_observer));
    return;
  }
  std::unique_ptr<ContentFrameSinkPrivate> content_frame_sink_private(
      new ContentFrameSinkPrivate(this, compositor_frame_sink_id,
                                  std::move(private_request),
                                  std::move(content_frame_sink_observer)));
  private_interfaces_[compositor_frame_sink_id] =
      std::move(content_frame_sink_private);
}

bool DisplayCompositorConnection::HasEncounteredError() const {
  return display_compositor_.encountered_error();
}

void DisplayCompositorConnection::CreateContentFrameSink(
    uint32_t client_id,
    int32_t sink_id,
    const gpu::SurfaceHandle& handle,
    mojom::LayerTreeSettingsPtr settings,
    mojom::ContentFrameSinkRequest content_frame_sink,
    mojom::ContentFrameSinkClientPtr content_frame_sink_client) {
  // There is a race between RegisterContentFrameSinkObserver and
  // CreateContentFrameSink. If RegisterContentFrameSinkObserver happens first
  // then we create a ContentFrameSinkPrivate interface object first. We look up
  // the private interface here and pass the InterfaceRequest end of the
  // MessagePipe over to the display compositor. If we don't yet have a private
  // interface object, then we create one and bind the privileged client in
  // RegisterContentFrameSinkObserver.
  CompositorFrameSinkId compositor_frame_sink_id(client_id, sink_id);
  auto it = private_interfaces_.find(compositor_frame_sink_id);
  cc::mojom::ContentFrameSinkPrivateRequest private_request;
  if (it != private_interfaces_.end() &&
      !it->second->HasPendingContentFrameSinkPrivateRequest()) {
    // We may have a stale ContentFrameSinkPrivate object. In this case,
    // we sever the connection and create a new ContentFrameSinkPrivate.
    it->second->OnClientConnectionLost();
    it = private_interfaces_.end();
  }
  if (it == private_interfaces_.end()) {
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

void DisplayCompositorConnection::OnContentFrameSinkPrivateConnectionLost(
    const CompositorFrameSinkId& compositor_frame_sink_id) {
  // This happens when the ContentFrameSinkObserver goes away.
  private_interfaces_.erase(compositor_frame_sink_id);
}

void DisplayCompositorConnection::OnContentFrameSinkClientConnectionLost(
    const cc::CompositorFrameSinkId& compositor_frame_sink_id) {
  auto it = private_interfaces_.find(compositor_frame_sink_id);
  if (it != private_interfaces_.end())
    it->second->OnClientConnectionLost();
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
