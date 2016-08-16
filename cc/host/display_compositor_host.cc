// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/host/display_compositor_host.h"

#include "cc/host/display_compositor_connection.h"

namespace cc {

void DisplayCompositorHost::Create(
    gpu::SurfaceHandle surface_handle,
    int32_t process_id,
    scoped_refptr<Delegate> delegate,
    mojom::DisplayCompositorHostRequest request) {
  fprintf(stderr, ">>>%s\n", __PRETTY_FUNCTION__);
  new DisplayCompositorHost(surface_handle, process_id, delegate,
                            std::move(request));
}

DisplayCompositorHost::~DisplayCompositorHost() = default;

void DisplayCompositorHost::CreateCompositorChannel(
    mojom::CompositorChannelRequest compositor_channel) {
  ConnectToDisplayCompositorIfNecessary();
  display_compositor_->CreateCompositorChannel(std::move(compositor_channel));
}

void DisplayCompositorHost::CreateCompositor(
    int32_t routing_id,
    mojom::LayerTreeSettingsPtr settings,
    mojom::CompositorRequest compositor,
    mojom::CompositorClientPtr client) {
  fprintf(stderr, ">>>>%s routing_id: %d", __PRETTY_FUNCTION__, routing_id);
  ConnectToDisplayCompositorIfNecessary();
  // TODO(fsamuel): (process_id, routing_id) uniquely identifies a
  // RenderWidgetHost and thus a RenderWidgetHostView and thus
  // a DelegatedFrameHost. We can map compositor_id => DelegatedFrameHost.
  display_compositor_->CreateCompositor(
      next_compositor_id_++, surface_handle_, std::move(settings),
      std::move(compositor), std::move(client));
}

DisplayCompositorHost::DisplayCompositorHost(
    gpu::SurfaceHandle surface_handle,
    int32_t process_id,
    scoped_refptr<Delegate> delegate,
    mojom::DisplayCompositorHostRequest request)
    : surface_handle_(surface_handle),
      process_id_(process_id),
      delegate_(delegate),
      binding_(this, std::move(request)) {}

void DisplayCompositorHost::ConnectToDisplayCompositorIfNecessary() {
  if (!display_compositor_)
    display_compositor_ = delegate_->GetDisplayCompositorConnection();
}

}  // namespace cc
