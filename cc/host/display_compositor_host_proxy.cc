// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/host/display_compositor_host_proxy.h"

#include "cc/host/display_compositor_host.h"

namespace cc {

DisplayCompositorHostProxy::~DisplayCompositorHostProxy() = default;

void DisplayCompositorHostProxy::RequestSurfaceManager(
    mojom::SurfaceManagerRequest surface_manager) {
  host_->RequestSurfaceManager(std::move(surface_manager));
}

void DisplayCompositorHostProxy::CreateContentFrameSink(
    int32_t routing_id,
    mojom::LayerTreeSettingsPtr settings,
    mojom::ContentFrameSinkRequest compositor_frame_sink,
    mojom::ContentFrameSinkClientPtr compositor_frame_sink_client) {
  host_->CreateContentFrameSink(routing_id, std::move(settings),
                                std::move(compositor_frame_sink),
                                std::move(compositor_frame_sink_client));
}

DisplayCompositorHostProxy::DisplayCompositorHostProxy(
    int process_id,
    cc::mojom::DisplayCompositorHostPtr host)
    : host_(std::move(host)) {}

}  // namespace cc
