// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_HOST_DISPLAY_COMPOSITOR_CONNECTION_H_
#define CC_HOST_DISPLAY_COMPOSITOR_CONNECTION_H_

#include "cc/host/content_frame_sink_private.h"
#include "cc/ipc/compositor.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace cc {

class DisplayCompositorConnectionClient;

// This class encapsulates a single two way connection to the display
// compositor. There is a DisplayCompositorHost object for each
// display compositor host <=> client connection.  However, all
// DisplayCompositorHost objects should share a single connection to the
// display compositor.
class DisplayCompositorConnection : public mojom::DisplayCompositorClient {
 public:
  DisplayCompositorConnection(
      DisplayCompositorConnectionClient* connection_client,
      mojom::DisplayCompositorPtr display_compositor,
      mojom::DisplayCompositorClientRequest display_compositor_client);

  ~DisplayCompositorConnection() override;

  void RegisterContentFrameSinkObserver(
      const CompositorFrameSinkId& compositor_frame_sink_id,
      cc::mojom::ContentFrameSinkPrivateRequest private_request,
      cc::mojom::ContentFrameSinkObserverPtr content_frame_sink_observer);

  bool HasEncounteredError() const;

  // cc::mojom::DisplayCompositor implementation:
  void CreateContentFrameSink(
      uint32_t client_id,
      int32_t sink_id,
      const gpu::SurfaceHandle& handle,
      mojom::LayerTreeSettingsPtr settings,
      mojom::ContentFrameSinkRequest content_frame_sink,
      mojom::ContentFrameSinkClientPtr content_frame_sink_client);

  void OnConnectionLost();

  void OnContentFrameSinkPrivateConnectionLost(
      const CompositorFrameSinkId& compositor_frame_sink_id);

  // cc::mojom::DisplayCompositorClient implementation:
  void OnContentFrameSinkClientConnectionLost(
      const cc::CompositorFrameSinkId& compositor_frame_sink_id) override;
  void OnSurfaceCreated(const gfx::Size& frame_size,
                        const cc::SurfaceId& surface_id) override;

 private:
  DisplayCompositorConnectionClient* const connection_client_;

  std::unordered_map<CompositorFrameSinkId,
                     std::unique_ptr<ContentFrameSinkPrivate>,
                     CompositorFrameSinkIdHash>
      private_interfaces_;

  mojom::DisplayCompositorPtr display_compositor_;
  mojo::Binding<mojom::DisplayCompositorClient> client_binding_;
  base::WeakPtrFactory<DisplayCompositorConnection> weak_factory_;
  DISALLOW_COPY_AND_ASSIGN(DisplayCompositorConnection);
};

}  // namespace cc

#endif  // CC_HOST_DISPLAY_COMPOSITOR_CONNECTION_H_
