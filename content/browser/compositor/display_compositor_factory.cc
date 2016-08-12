// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/compositor/display_compositor_factory.h"

#include "content/browser/gpu/gpu_process_host.h"
#include "services/shell/public/cpp/interface_provider.h"

namespace content {

DisplayCompositorFactory::DisplayCompositorFactory() {}

cc::DisplayCompositorConnection
DisplayCompositorFactory::GetDisplayCompositorConnection() {
  if (!display_compositor_factory_) {
    GpuProcessHost* host =
        GpuProcessHost::Get(GpuProcessHost::GPU_PROCESS_KIND_SANDBOXED,
                            CAUSE_FOR_GPU_LAUNCH_BROWSER_STARTUP);

    host->GetRemoteInterfaces()->GetInterface(&display_compositor_factory_);
  }

  cc::mojom::DisplayCompositorClientPtr display_compositor_client;
  cc::DisplayCompositorConnection connection;
  connection.client_request = mojo::GetProxy(&display_compositor_client);
  display_compositor_factory_->CreateDisplayCompositor(
      mojo::GetProxy(&connection.compositor),
      std::move(display_compositor_client));
  return connection;
}

DisplayCompositorFactory::~DisplayCompositorFactory() = default;

}  // namespace content
