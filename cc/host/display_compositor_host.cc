// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/host/display_compositor_host.h"

#include "cc/host/display_compositor_connection.h"

namespace cc {

void DisplayCompositorHost::Create(
    int32_t client_id,
    scoped_refptr<DisplayCompositorConnectionFactory> connection_factory,
    mojom::DisplayCompositorHostRequest request) {
  fprintf(stderr, ">>>%s\n", __PRETTY_FUNCTION__);
  new DisplayCompositorHost(client_id, connection_factory, std::move(request),
                            nullptr);
}

void DisplayCompositorHost::CreatePrivate(
    int32_t client_id,
    scoped_refptr<DisplayCompositorConnectionFactory> connection_factory,
    mojom::DisplayCompositorHostRequest request,
    mojom::DisplayCompositorHostPrivateRequest private_request) {
  fprintf(stderr, ">>>%s\n", __PRETTY_FUNCTION__);
  new DisplayCompositorHost(client_id, connection_factory, std::move(request),
                            std::move(private_request));
}

DisplayCompositorHost::~DisplayCompositorHost() = default;

void DisplayCompositorHost::AddRefOnSurfaceId(const SurfaceId& id) {
  ConnectToDisplayCompositorIfNecessary();
  display_compositor_->AddRefOnSurfaceId(id);
}

void DisplayCompositorHost::MoveTempRefToRefOnSurfaceId(const SurfaceId& id) {
  ConnectToDisplayCompositorIfNecessary();
  display_compositor_->MoveTempRefToRefOnSurfaceId(id);
}

void DisplayCompositorHost::RegisterClientHierarchy(uint32_t parent_client_id,
                                                    uint32_t child_client_id) {
  ConnectToDisplayCompositorIfNecessary();
  display_compositor_->RegisterClientHierarchy(parent_client_id,
                                               child_client_id);
}

void DisplayCompositorHost::UnregisterClientHierarchy(
    uint32_t parent_client_id,
    uint32_t child_client_id) {
  ConnectToDisplayCompositorIfNecessary();
  display_compositor_->UnregisterClientHierarchy(parent_client_id,
                                                 child_client_id);
}

void DisplayCompositorHost::CreateContentFrameSink(
    int32_t sink_id,
    mojom::LayerTreeSettingsPtr settings,
    mojom::ContentFrameSinkRequest content_frame_sink,
    mojom::ContentFrameSinkClientPtr content_frame_sink_client) {
  fprintf(stderr, ">>>>%s sink_id: %d", __PRETTY_FUNCTION__, sink_id);
  // TODO(fsamuel): (client_id = process_id, sink_id = routing_id)  uniquely
  // identifies a RenderWidgetHost and thus a RenderWidgetHostView and thus
  // a DelegatedFrameHost. We can map compositor_id => DelegatedFrameHost.
  CreateContentFrameSinkWithHandle(
      sink_id, gpu::kNullSurfaceHandle, std::move(settings),
      std::move(content_frame_sink), std::move(content_frame_sink_client));
}

void DisplayCompositorHost::CreateContentFrameSinkWithHandle(
    int32_t sink_id,
    const gpu::SurfaceHandle& surface_handle,
    mojom::LayerTreeSettingsPtr settings,
    mojom::ContentFrameSinkRequest content_frame_sink,
    mojom::ContentFrameSinkClientPtr content_frame_sink_client) {
  ConnectToDisplayCompositorIfNecessary();
  // TODO(fsamuel): (client_id, routing_id) uniquely identifies a
  // RenderWidgetHost and thus a RenderWidgetHostView and thus
  // a DelegatedFrameHost. We can map compositor_id => DelegatedFrameHost.
  display_compositor_->CreateContentFrameSink(
      client_id_, sink_id, surface_handle, std::move(settings),
      std::move(content_frame_sink), std::move(content_frame_sink_client));
}

DisplayCompositorHost::DisplayCompositorHost(
    int32_t client_id,
    scoped_refptr<DisplayCompositorConnectionFactory> connection_factory,
    mojom::DisplayCompositorHostRequest request,
    mojom::DisplayCompositorHostPrivateRequest private_request)
    : client_id_(client_id),
      connection_factory_(connection_factory),
      binding_(this, std::move(request)),
      private_binding_(this, std::move(private_request)) {}

void DisplayCompositorHost::ConnectToDisplayCompositorIfNecessary() {
  if (!display_compositor_)
    display_compositor_ = connection_factory_->GetDisplayCompositorConnection();
}

}  // namespace cc
