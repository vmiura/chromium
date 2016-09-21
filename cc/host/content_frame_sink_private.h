// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_HOST_CONTENT_FRAME_SINK_PRIVATE_
#define CC_HOST_CONTENT_FRAME_SINK_PRIVATE_

#include "cc/ipc/compositor.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace cc {

class DisplayCompositorConnection;

class ContentFrameSinkPrivate : public mojom::ContentFrameSinkPrivate,
                                public mojom::ContentFrameSinkObserver {
 public:
  // Use this constructor if a privileged DisplayCompositorHost client
  // Registers a ContentFrameSink observer before the ContentFrameSink
  // has been created.
  ContentFrameSinkPrivate(
      DisplayCompositorConnection* display_compositor_connection,
      const CompositorFrameSinkId& compositor_frame_sink_id,
      cc::mojom::ContentFrameSinkPrivateRequest request,
      cc::mojom::ContentFrameSinkObserverPtr display_compositor_client);

  // Use this constructor if a client creates a ContentFrameSink before
  // the privileged client registers a ContentFrameSink observer.
  ContentFrameSinkPrivate(
      DisplayCompositorConnection* display_compositor_connection,
      const CompositorFrameSinkId& compositor_frame_sink_id,
      cc::mojom::ContentFrameSinkPrivatePtr content_frame_sink_private);

  ~ContentFrameSinkPrivate() override;

  void BindPrivilegedClient(
      cc::mojom::ContentFrameSinkPrivateRequest request,
      cc::mojom::ContentFrameSinkObserverPtr content_frame_sink_observer);

  bool HasPendingContentFrameSinkPrivateRequest();

  cc::mojom::ContentFrameSinkPrivateRequest GetContentFrameSinkPrivateRequest();

  // cc::mojom::ContentFrameSinkPrivate implementation.
  void AddRefOnSurfaceId(const SurfaceId& id) override;
  void TransferRef(const SurfaceId& id) override;
  void RegisterChildSink(const CompositorFrameSinkId& child_client_id) override;
  void UnregisterChildSink(
      const CompositorFrameSinkId& child_client_id) override;

  // cc::mojom::ContentFrameSinkObserver implementation:
  void OnClientConnectionLost() override;
  void OnSurfaceCreated(const gfx::Size& frame_size,
                        const cc::SurfaceId& surface_id) override;

 private:
  void OnObserverLost();

  DisplayCompositorConnection* const display_compositor_connection_;

  const CompositorFrameSinkId compositor_frame_sink_id_;

  // This is an InterfacePtr to the privileged client holding the
  // ContentFrameSinkPrivate interface.
  cc::mojom::ContentFrameSinkObserverPtr content_frame_sink_observer_;

  // This is the pending private request to pass on to the display compositor
  // once a ContentFrameSink with the provided CompositorFrameSinkId has been
  // requested by some client.
  cc::mojom::ContentFrameSinkPrivateRequest pending_private_request_;

  // This is the InterfacePtr to the ContentFrameSink in the display compositor.
  // This object will go away if a connection error is detected on this
  // interface pointer.
  cc::mojom::ContentFrameSinkPrivatePtr content_frame_sink_private_;

  gfx::Size last_frame_size_;
  SurfaceId last_surface_id_;

  // This is a connection from the privileged client.
  mojo::Binding<cc::mojom::ContentFrameSinkPrivate> binding_;

  DISALLOW_COPY_AND_ASSIGN(ContentFrameSinkPrivate);
};

}  // namespace cc

#endif  // CC_HOST_CONTENT_FRAME_SINK_PRIVATE_
