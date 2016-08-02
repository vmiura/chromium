// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_CLIENT_COMPOSITOR_CHANNEL_HOST_H_
#define CC_CLIENT_COMPOSITOR_CHANNEL_HOST_H_

#include "cc/client/client_export.h"
#include "cc/ipc/compositor.mojom.h"
#include "cc/client/compositor_proxy.h"

namespace gpu {
class GpuChannelHost;
}

namespace cc {

class CC_CLIENT_EXPORT CompositorChannelHost {
 public:
  static std::unique_ptr<CompositorChannelHost> Create(
      gpu::GpuChannelHost* host);

  ~CompositorChannelHost();

  std::unique_ptr<CompositorProxy> CreateCompositor();

 private:
  CompositorChannelHost(gpu::GpuChannelHost* host);

  cc::mojom::CompositorFactoryAssociatedPtr compositor_factory_;
  DISALLOW_COPY_AND_ASSIGN(CompositorChannelHost);
};

}  // namespace cc

#endif  // CC_CLIENT_COMPOSITOR_CHANNEL_HOST_H_
