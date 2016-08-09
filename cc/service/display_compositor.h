// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_SERVICE_DISPLAY_COMPOSITOR_H_
#define CC_SERVICE_DISPLAY_COMPOSITOR_H_

#include "base/containers/scoped_ptr_hash_map.h"
#include "cc/ipc/compositor.mojom.h"
#include "cc/raster/single_thread_task_graph_runner.h"
#include "cc/service/compositor_channel.h"
#include "cc/surfaces/surface_manager.h"
#include "gpu/ipc/common/surface_handle.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace gpu {

class GpuMemoryBufferManager;
class ImageFactory;
class SyncPointManager;

namespace gles2 {
class MailboxManager;
}
}

namespace cc {

class LayerTreeSettings;
class SharedBitmapManager;

class DisplayCompositor : public mojom::DisplayCompositor {
 public:
  // TODO(fsamuel): Merge ServiceFactory and DisplayCompositor.
  DisplayCompositor(
      ServiceFactory* factory,
      mojom::DisplayCompositorRequest request,
      mojom::DisplayCompositorClientPtr client,
      scoped_refptr<base::SingleThreadTaskRunner> compositor_task_runner);

  ~DisplayCompositor() override;

  // mojom::DisplayCompositor implementation.
  void CreateCompositor(uint32_t client_id,
                        const gpu::SurfaceHandle& handle,
                        mojom::LayerTreeSettingsPtr settings,
                        mojom::CompositorRequest compositor,
                        mojom::CompositorClientPtr compositor_client) override;

 private:
  ServiceFactory* const factory_;

  scoped_refptr<base::SingleThreadTaskRunner> compositor_task_runner_;

  mojom::DisplayCompositorClientPtr client_;
  mojo::Binding<mojom::DisplayCompositor> binding_;
  DISALLOW_COPY_AND_ASSIGN(DisplayCompositor);
};

}  // namespace cc

#endif  // CC_SERVICE_DISPLAY_COMPOSITOR_H_
