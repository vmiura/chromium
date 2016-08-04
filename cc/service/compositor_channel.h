// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_SERVICE_COMPOSITOR_CHANNEL_
#define CC_SERVICE_COMPOSITOR_CHANNEL_

#include "base/containers/scoped_ptr_hash_map.h"
#include "cc/ipc/compositor.mojom.h"
#include "mojo/public/cpp/bindings/associated_binding.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace gpu {
class GpuChannel;
}

namespace cc {
class LayerTreeSettings;
class ServiceFactory;

class CompositorChannel : public cc::mojom::CompositorFactory {
 public:
  CompositorChannel(ServiceFactory* factory, gpu::GpuChannel* channel,
                    scoped_refptr<base::SingleThreadTaskRunner> compositor_task_runner);
  ~CompositorChannel() override;

  // cc::mojom::Compositor implementation.
  void CreateCompositor(
      const gpu::SurfaceHandle& handle,
      cc::mojom::LayerTreeSettingsPtr settings,
      cc::mojom::CompositorRequest compositor,
      cc::mojom::CompositorClientPtr compositor_client) override;

  ServiceFactory* factory() { return factory_; }

 private:
  void BindCompositorFactoryRequest(
      cc::mojom::CompositorFactoryAssociatedRequest request);

  ServiceFactory* const factory_;
  gpu::GpuChannel* const channel_;
  mojo::AssociatedBinding<cc::mojom::CompositorFactory> binding_;
  scoped_refptr<base::SingleThreadTaskRunner> compositor_task_runner_;
  DISALLOW_COPY_AND_ASSIGN(CompositorChannel);
};

}  // namespace cc

#endif  // CC_SERVICE_COMPOSITOR_CHANNEL_H_
