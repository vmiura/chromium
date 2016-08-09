// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/host/display_compositor_host.h"

namespace cc {

DisplayCompositorConnection::DisplayCompositorConnection() = default;

DisplayCompositorConnection::DisplayCompositorConnection(
    DisplayCompositorConnection&& other) = default;

DisplayCompositorConnection::~DisplayCompositorConnection() = default;

void DisplayCompositorHost::Create(
    int32_t process_id,
    std::unique_ptr<Delegate> delegate,
    mojom::DisplayCompositorHostRequest request) {
  fprintf(stderr, ">>>%s\n", __PRETTY_FUNCTION__);
  new DisplayCompositorHost(process_id, std::move(delegate),
                            std::move(request));
}

DisplayCompositorHost::~DisplayCompositorHost() = default;

void DisplayCompositorHost::CreateCompositor(
    int32_t routing_id,
    mojom::LayerTreeSettingsPtr settings,
    mojom::CompositorRequest compositor,
    mojom::CompositorClientPtr client) {
  fprintf(stderr, ">>>>%s routing_id: %d", __PRETTY_FUNCTION__, routing_id);
  // TODO(fsamuel): (process_id, routing_id) uniquely identifies a
  // RenderWidgetHost and thus a RenderWidgetHostView and thus
  // a DelegatedFrameHost. We can map compositor_id => DelegatedFrameHost.
  if (!display_compositor_) {
    DisplayCompositorConnection connection =
        delegate_->GetDisplayCompositorConnection();
    display_compositor_ = std::move(connection.compositor);
    client_binding_.Bind(std::move(connection.client_request));
  }
  display_compositor_->CreateCompositor(
      next_compositor_id_++, gpu::kNullSurfaceHandle, std::move(settings),
      std::move(compositor), std::move(client));
}

DisplayCompositorHost::DisplayCompositorHost(
    int32_t process_id,
    std::unique_ptr<Delegate> delegate,
    mojom::DisplayCompositorHostRequest request)
    : process_id_(process_id),
      delegate_(std::move(delegate)),
      client_binding_(this),
      binding_(this, std::move(request)) {}

}  // namespace cc
