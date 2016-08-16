// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_SERVICE_DISPLAY_COMPOSITOR_H_
#define CC_SERVICE_DISPLAY_COMPOSITOR_H_

#include "base/containers/scoped_ptr_hash_map.h"
#include "cc/ipc/compositor.mojom.h"
#include "cc/raster/single_thread_task_graph_runner.h"
#include "cc/surfaces/surface_manager.h"
#include "gpu/ipc/common/surface_handle.h"
#include "mojo/public/cpp/bindings/binding_set.h"
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
class ServiceFactory;
class SharedBitmapManager;

class DisplayCompositor : public mojom::SurfaceManager,
                          public mojom::DisplayCompositor,
                          public SurfaceManager::Delegate {
 public:
  // TODO(fsamuel): Merge ServiceFactory and DisplayCompositor.
  DisplayCompositor(
      ServiceFactory* factory,
      mojom::DisplayCompositorRequest request,
      mojom::DisplayCompositorClientPtr client,
      scoped_refptr<base::SingleThreadTaskRunner> compositor_task_runner);

  ~DisplayCompositor() override;

  // mojom::DisplayCompositor implementation.
  void RequestSurfaceManager(
      mojom::SurfaceManagerRequest surface_manager) override;
  void CreateContentFrameSink(
      uint32_t client_id,
      const gpu::SurfaceHandle& handle,
      mojom::LayerTreeSettingsPtr settings,
      mojom::ContentFrameSinkRequest content_frame_sink,
      mojom::ContentFrameSinkClientPtr content_frame_sink_client) override;

  // mojom::SurfaceManager implementation.
  void AddRefOnSurfaceId(const SurfaceId& id) override;
  void MoveTempRefToRefOnSurfaceId(const SurfaceId& id) override;

  // SurfaceManager::Delegate implementation.
  void OnSurfaceCreated(const gfx::Size& frame_size,
                        const SurfaceId& surface_id) override;

 private:
  ServiceFactory* const factory_;

  scoped_refptr<base::SingleThreadTaskRunner> compositor_task_runner_;

  int next_service_id_ = 1;
  SingleThreadTaskGraphRunner task_graph_runner_;
  cc::SurfaceManager surface_manager_;
  mojom::DisplayCompositorClientPtr client_;
  mojo::BindingSet<mojom::SurfaceManager> surface_manager_bindings_;
  mojo::Binding<mojom::DisplayCompositor> display_compositor_binding_;
  DISALLOW_COPY_AND_ASSIGN(DisplayCompositor);
};

}  // namespace cc

#endif  // CC_SERVICE_DISPLAY_COMPOSITOR_H_
