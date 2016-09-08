// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/host/content_frame_sink_private.h"

#include "cc/host/display_compositor_connection.h"

namespace cc {

ContentFrameSinkPrivate::ContentFrameSinkPrivate(
    DisplayCompositorConnection* display_compositor_connection,
    const CompositorFrameSinkId& compositor_frame_sink_id,
    cc::mojom::ContentFrameSinkPrivateRequest request,
    cc::mojom::DisplayCompositorClientPtr display_compositor_client)
    : display_compositor_connection_(display_compositor_connection),
      compositor_frame_sink_id_(compositor_frame_sink_id),
      display_compositor_client_(std::move(display_compositor_client)),
      binding_(this, std::move(request)) {
  binding_.set_connection_error_handler(base::Bind(
      &ContentFrameSinkPrivate::OnConnectionLost, base::Unretained(this)));

  pending_private_request_ = mojo::GetProxy(&content_frame_sink_private_);
}

ContentFrameSinkPrivate::ContentFrameSinkPrivate(
    DisplayCompositorConnection* display_compositor_connection,
    const CompositorFrameSinkId& compositor_frame_sink_id,
    cc::mojom::ContentFrameSinkPrivatePtr content_frame_sink_private)
    : display_compositor_connection_(display_compositor_connection),
      compositor_frame_sink_id_(compositor_frame_sink_id),
      content_frame_sink_private_(std::move(content_frame_sink_private)),
      binding_(this) {}

ContentFrameSinkPrivate::~ContentFrameSinkPrivate() {}

void ContentFrameSinkPrivate::BindPrivilegedClient(
    cc::mojom::ContentFrameSinkPrivateRequest request,
    cc::mojom::DisplayCompositorClientPtr display_compositor_client) {
  DCHECK(!binding_.is_bound());
  binding_.Bind(std::move(request));
  binding_.set_connection_error_handler(base::Bind(
      &ContentFrameSinkPrivate::OnConnectionLost, base::Unretained(this)));
  display_compositor_client_ = std::move(display_compositor_client);
  if (!last_surface_id_.is_null()) {
    display_compositor_client_->OnSurfaceCreated(last_frame_size_,
                                                 last_surface_id_);
  }
}

cc::mojom::ContentFrameSinkPrivateRequest
ContentFrameSinkPrivate::GetContentFrameSinkPrivateRequest() {
  return std::move(pending_private_request_);
}

void ContentFrameSinkPrivate::AddRefOnSurfaceId(const SurfaceId& id) {
  content_frame_sink_private_->AddRefOnSurfaceId(id);
}

void ContentFrameSinkPrivate::TransferRef(const SurfaceId& id) {
  content_frame_sink_private_->TransferRef(id);
}

void ContentFrameSinkPrivate::RegisterChildSink(
    const CompositorFrameSinkId& child_client_id) {
  content_frame_sink_private_->RegisterChildSink(child_client_id);
}

void ContentFrameSinkPrivate::UnregisterChildSink(
    const CompositorFrameSinkId& child_client_id) {
  content_frame_sink_private_->UnregisterChildSink(child_client_id);
}

void ContentFrameSinkPrivate::OnSurfaceCreated(
    const gfx::Size& frame_size,
    const cc::SurfaceId& surface_id) {
  // TODO(fsamuel): We should probably return the last_surface_id back to the
  // gpu immediately if we don't have a display_compositor_client_ or else we
  // will leak refs (because we only keep track of the last surface ID).
  last_frame_size_ = frame_size;
  last_surface_id_ = surface_id;
  if (display_compositor_client_)
    display_compositor_client_->OnSurfaceCreated(frame_size, surface_id);
}

void ContentFrameSinkPrivate::OnConnectionLost() {
  display_compositor_connection_->OnPrivateConnectionLost(
      compositor_frame_sink_id_);
}

}  // namespace cc
