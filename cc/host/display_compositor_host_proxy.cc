// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/host/display_compositor_host_proxy.h"

#include "cc/host/display_compositor_host.h"

namespace cc {

DisplayCompositorHostProxy::~DisplayCompositorHostProxy() = default;

void DisplayCompositorHostProxy::CreateCompositorChannel(
    mojom::CompositorChannelRequest compositor_channel) {
  host_->CreateCompositorChannel(std::move(compositor_channel));
}

void DisplayCompositorHostProxy::CreateCompositor(
    int32_t routing_id,
    mojom::LayerTreeSettingsPtr settings,
    mojom::CompositorRequest compositor,
    mojom::CompositorClientPtr client) {
  host_->CreateCompositor(routing_id, std::move(settings),
                          std::move(compositor), std::move(client));
}

DisplayCompositorHostProxy::DisplayCompositorHostProxy(
    int process_id,
    cc::mojom::DisplayCompositorHostPtr host)
    : host_(std::move(host)) {}

}  // namespace cc
