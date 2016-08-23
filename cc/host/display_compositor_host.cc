// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/host/display_compositor_host.h"

#include "cc/host/display_compositor_connection.h"

namespace cc {

void DisplayCompositorHost::Create(
    int32_t process_id,
    scoped_refptr<DisplayCompositorConnectionFactory> connection_factory,
    mojom::DisplayCompositorHostRequest request) {
  fprintf(stderr, ">>>%s\n", __PRETTY_FUNCTION__);
  new DisplayCompositorHost(process_id, connection_factory,
                            std::move(request), nullptr);
}

void DisplayCompositorHost::CreatePrivate(
    int32_t process_id,
    scoped_refptr<DisplayCompositorConnectionFactory> connection_factory,
    mojom::DisplayCompositorHostRequest request,
    mojom::DisplayCompositorHostPrivateRequest private_request) {
  fprintf(stderr, ">>>%s\n", __PRETTY_FUNCTION__);
  new DisplayCompositorHost(process_id, connection_factory,
                            std::move(request), std::move(private_request));
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
    int32_t routing_id,
    mojom::LayerTreeSettingsPtr settings,
    mojom::ContentFrameSinkRequest content_frame_sink,
    mojom::ContentFrameSinkClientPtr content_frame_sink_client) {
  fprintf(stderr, ">>>>%s routing_id: %d", __PRETTY_FUNCTION__, routing_id);
  // TODO(fsamuel): (process_id, routing_id) uniquely identifies a
  // RenderWidgetHost and thus a RenderWidgetHostView and thus
  // a DelegatedFrameHost. We can map compositor_id => DelegatedFrameHost.
  CreateContentFrameSinkWithHandle(gpu::kNullSurfaceHandle, std::move(settings),
                                   std::move(content_frame_sink),
                                   std::move(content_frame_sink_client));
}

void DisplayCompositorHost::CreateContentFrameSinkWithHandle(
    const gpu::SurfaceHandle& surface_handle,
    mojom::LayerTreeSettingsPtr settings,
    mojom::ContentFrameSinkRequest content_frame_sink,
    mojom::ContentFrameSinkClientPtr content_frame_sink_client) {
  ConnectToDisplayCompositorIfNecessary();
  // TODO(fsamuel): (process_id, routing_id) uniquely identifies a
  // RenderWidgetHost and thus a RenderWidgetHostView and thus
  // a DelegatedFrameHost. We can map compositor_id => DelegatedFrameHost.
  display_compositor_->CreateContentFrameSink(
      next_compositor_id_++, surface_handle, std::move(settings),
      std::move(content_frame_sink), std::move(content_frame_sink_client));
}

DisplayCompositorHost::DisplayCompositorHost(
    int32_t process_id,
    scoped_refptr<DisplayCompositorConnectionFactory> connection_factory,
    mojom::DisplayCompositorHostRequest request,
    mojom::DisplayCompositorHostPrivateRequest private_request)
    : process_id_(process_id),
      connection_factory_(connection_factory),
      binding_(this, std::move(request)),
      private_binding_(this, std::move(private_request)) {}

void DisplayCompositorHost::ConnectToDisplayCompositorIfNecessary() {
  if (!display_compositor_)
    display_compositor_ = connection_factory_->GetDisplayCompositorConnection();
}

}  // namespace cc
