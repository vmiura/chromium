// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_HOST_DISPLAY_COMPOSITOR_HOST_H_
#define CC_HOST_DISPLAY_COMPOSITOR_HOST_H_

#include "cc/host/display_compositor_connection_factory.h"
#include "cc/ipc/compositor.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace cc {

class DisplayCompositorConnection;

class DisplayCompositorHost : public mojom::DisplayCompositorHost,
                              public mojom::DisplayCompositorHostPrivate {
 public:
  // Create on IO thread.
  static void Create(
      int32_t client_id,
      scoped_refptr<DisplayCompositorConnectionFactory> connection_factory,
      mojom::DisplayCompositorHostRequest request);

  static void CreatePrivate(
      int32_t client_id,
      scoped_refptr<DisplayCompositorConnectionFactory> connection_factory,
      mojom::DisplayCompositorHostRequest request,
      mojom::DisplayCompositorHostPrivateRequest private_request);

  ~DisplayCompositorHost() override;

  int32_t client_id() const { return client_id_; }

  // DisplayCompositorHost implementation.
  void CreateContentFrameSink(
      int32_t sink_id,
      mojom::LayerTreeSettingsPtr settings,
      mojom::ContentFrameSinkRequest content_frame_sink,
      mojom::ContentFrameSinkClientPtr content_frame_sink_client) override;

  // DisplayCompositorHostPrivate implementation.
  // TODO(fsamuel): Need to set manifest indicating that only the browser
  // can access the private interface.
  void CreateContentFrameSinkWithHandle(
      int32_t sink_id,
      const gpu::SurfaceHandle& surface_handle,
      mojom::LayerTreeSettingsPtr settings,
      mojom::ContentFrameSinkRequest content_frame_sink,
      mojom::ContentFrameSinkClientPtr content_frame_sink_client) override;
  void RegisterContentFrameSinkObserver(
      const CompositorFrameSinkId& compositor_frame_sink_id,
      mojom::ContentFrameSinkPrivateRequest content_frame_sink_private,
      mojom::ContentFrameSinkObserverPtr content_frame_sink_observer) override;

 private:
  DisplayCompositorHost(
      int32_t client_id,
      scoped_refptr<DisplayCompositorConnectionFactory> connection_factory,
      mojom::DisplayCompositorHostRequest request,
      mojom::DisplayCompositorHostPrivateRequest private_request);

  DisplayCompositorConnection* GetDisplayCompositorConnection();

  const int32_t client_id_;
  scoped_refptr<DisplayCompositorConnectionFactory> connection_factory_;
  mojo::StrongBinding<mojom::DisplayCompositorHost> binding_;
  mojo::Binding<mojom::DisplayCompositorHostPrivate> private_binding_;

  DISALLOW_COPY_AND_ASSIGN(DisplayCompositorHost);
};

}  // namespace cc

#endif  // CC_HOST_DISPLAY_COMPOSITOR_HOST_H_
