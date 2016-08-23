// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/host/display_compositor_connection.h"

namespace cc {

DisplayCompositorConnection::DisplayCompositorConnection(
    mojom::DisplayCompositorPtr display_compositor,
    mojom::DisplayCompositorClientRequest display_compositor_client)
    : display_compositor_(std::move(display_compositor)),
      client_binding_(this, std::move(display_compositor_client)) {}

DisplayCompositorConnection::~DisplayCompositorConnection() = default;

void DisplayCompositorConnection::AddObserver(
    DisplayCompositorConnectionObserver* observer) {
  observers_.AddObserver(observer);
}

void DisplayCompositorConnection::RemoveObserver(
    DisplayCompositorConnectionObserver* observer) {
  observers_.RemoveObserver(observer);
}

void DisplayCompositorConnection::AddRefOnSurfaceId(const SurfaceId& id) {
  display_compositor_->AddRefOnSurfaceId(id);
}

void DisplayCompositorConnection::MoveTempRefToRefOnSurfaceId(
    const SurfaceId& id) {
  display_compositor_->MoveTempRefToRefOnSurfaceId(id);
}

void DisplayCompositorConnection::RegisterClientHierarchy(
    uint32_t parent_client_id,
    uint32_t child_client_id) {
  display_compositor_->RegisterClientHierarchy(parent_client_id,
                                               child_client_id);
}

void DisplayCompositorConnection::UnregisterClientHierarchy(
    uint32_t parent_client_id,
    uint32_t child_client_id) {
  display_compositor_->UnregisterClientHierarchy(parent_client_id,
                                                 child_client_id);
}

void DisplayCompositorConnection::CreateContentFrameSink(
    uint32_t client_id,
    int32_t sink_id,
    const gpu::SurfaceHandle& handle,
    mojom::LayerTreeSettingsPtr settings,
    mojom::ContentFrameSinkRequest content_frame_sink,
    mojom::ContentFrameSinkClientPtr content_frame_sink_client) {
  display_compositor_->CreateContentFrameSink(
      client_id, sink_id, handle, std::move(settings),
      std::move(content_frame_sink), std::move(content_frame_sink_client));
}

void DisplayCompositorConnection::OnSurfaceCreated(
    const gfx::Size& frame_size,
    const cc::SurfaceId& surface_id) {
  // TODO(fsamuel): Adding an observer list here and in ConnectionFactory is
  // probably overkill. This should be cleaned up.
  FOR_EACH_OBSERVER(DisplayCompositorConnectionObserver, observers_,
                    OnSurfaceCreated(frame_size, surface_id));
}

}  // namespace cc
