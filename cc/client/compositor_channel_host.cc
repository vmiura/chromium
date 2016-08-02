// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/client/compositor_channel_host.h"

#include "base/memory/ptr_util.h"
#include "gpu/ipc/client/gpu_channel_host.h"

namespace cc {

CompositorChannelHost::~CompositorChannelHost() = default;

// static
std::unique_ptr<CompositorChannelHost> CompositorChannelHost::Create(
    gpu::GpuChannelHost* host) {
  return base::WrapUnique(new CompositorChannelHost(host));
}

std::unique_ptr<CompositorProxy> CompositorChannelHost::CreateCompositor() {
  cc::mojom::CompositorPtr compositor;
  cc::mojom::CompositorClientPtr compositor_client;
  cc::mojom::CompositorClientRequest compositor_client_request =
      mojo::GetProxy(&compositor_client);
  compositor_factory_->CreateCompositor(mojo::GetProxy(&compositor),
                                        std::move(compositor_client));
  return base::MakeUnique<CompositorProxy>(
      std::move(compositor), std::move(compositor_client_request));
}

CompositorChannelHost::CompositorChannelHost(gpu::GpuChannelHost* host) {
  host->GetRemoteAssociatedInterface(&compositor_factory_);
}

}  // namespace cc
