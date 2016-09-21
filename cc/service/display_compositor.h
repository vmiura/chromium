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

class ContentFrameSink;
class LayerTreeSettings;
class DisplayCompositorFactory;
class SharedBitmapManager;

class DisplayCompositor : public mojom::DisplayCompositor,
                          public SurfaceManager::Delegate {
 public:
  // TODO(fsamuel): Merge DisplayCompositorFactory and DisplayCompositor.
  DisplayCompositor(DisplayCompositorFactory* factory,
                    mojom::DisplayCompositorRequest request,
                    mojom::DisplayCompositorClientPtr client);

  ~DisplayCompositor() override;

  // mojom::DisplayCompositor implementation.
  void CreateContentFrameSink(
      uint32_t client_id,
      int32_t sink_id,
      const gpu::SurfaceHandle& handle,
      mojom::LayerTreeSettingsPtr settings,
      mojom::ContentFrameSinkRequest content_frame_sink,
      mojom::ContentFrameSinkPrivateRequest content_frame_sink_private,
      mojom::ContentFrameSinkClientPtr content_frame_sink_client) override;

  // SurfaceManager::Delegate implementation.
  void OnSurfaceCreated(const gfx::Size& frame_size,
                        const SurfaceId& surface_id) override;

  void OnContentFrameSinkClientConnectionLost(
      const cc::CompositorFrameSinkId& compositor_frame_sink_id,
      bool destroy_content_frame_sink);
  void OnContentFrameSinkPrivateConnectionLost(
      const cc::CompositorFrameSinkId& compositor_frame_sink_id,
      bool destroy_content_frame_sink);

 private:
  DisplayCompositorFactory* const factory_;

  scoped_refptr<base::SingleThreadTaskRunner> compositor_task_runner_;

  std::unordered_map<CompositorFrameSinkId,
                     std::unique_ptr<ContentFrameSink>,
                     CompositorFrameSinkIdHash>
      content_frame_sinks_;
  SingleThreadTaskGraphRunner task_graph_runner_;
  cc::SurfaceManager surface_manager_;
  mojom::DisplayCompositorClientPtr client_;
  mojo::Binding<mojom::DisplayCompositor> display_compositor_binding_;
  DISALLOW_COPY_AND_ASSIGN(DisplayCompositor);
};

}  // namespace cc

#endif  // CC_SERVICE_DISPLAY_COMPOSITOR_H_
