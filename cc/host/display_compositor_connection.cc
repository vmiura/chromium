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

void DisplayCompositorConnection::CreateCompositorChannel(
    mojom::CompositorChannelRequest compositor_channel) {
  display_compositor_->CreateCompositorChannel(std::move(compositor_channel));
}

void DisplayCompositorConnection::CreateCompositor(
    uint32_t client_id,
    const gpu::SurfaceHandle& handle,
    mojom::LayerTreeSettingsPtr settings,
    mojom::CompositorRequest compositor,
    mojom::CompositorClientPtr compositor_client) {
  display_compositor_->CreateCompositor(client_id, handle, std::move(settings),
                                        std::move(compositor),
                                        std::move(compositor_client));
}

}  // namespace cc
