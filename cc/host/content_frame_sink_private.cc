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
    cc::mojom::ContentFrameSinkObserverPtr content_frame_sink_observer)
    : display_compositor_connection_(display_compositor_connection),
      compositor_frame_sink_id_(compositor_frame_sink_id),
      content_frame_sink_observer_(std::move(content_frame_sink_observer)),
      binding_(this, std::move(request)) {
  binding_.set_connection_error_handler(base::Bind(
      &ContentFrameSinkPrivate::OnObserverLost, base::Unretained(this)));

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

ContentFrameSinkPrivate::~ContentFrameSinkPrivate() {
  if (content_frame_sink_observer_)
    content_frame_sink_observer_->OnConnectionLost();
}

void ContentFrameSinkPrivate::BindPrivilegedClient(
    cc::mojom::ContentFrameSinkPrivateRequest request,
    cc::mojom::ContentFrameSinkObserverPtr content_frame_sink_observer) {
  DCHECK(!binding_.is_bound());
  binding_.Bind(std::move(request));
  binding_.set_connection_error_handler(base::Bind(
      &ContentFrameSinkPrivate::OnObserverLost, base::Unretained(this)));
  content_frame_sink_observer_ = std::move(content_frame_sink_observer);
  if (!last_surface_id_.is_null()) {
    content_frame_sink_observer_->OnSurfaceCreated(last_frame_size_,
                                                   last_surface_id_);
  }
}

bool ContentFrameSinkPrivate::HasPendingContentFrameSinkPrivateRequest() {
  return pending_private_request_.is_pending();
}

cc::mojom::ContentFrameSinkPrivateRequest
ContentFrameSinkPrivate::GetContentFrameSinkPrivateRequest() {
  DCHECK(pending_private_request_.is_pending());
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
  // gpu immediately if we don't have a content_frame_sink_observer_ or else we
  // will leak refs (because we only keep track of the last surface ID).
  last_frame_size_ = frame_size;
  last_surface_id_ = surface_id;
  if (content_frame_sink_observer_)
    content_frame_sink_observer_->OnSurfaceCreated(frame_size, surface_id);
}

void ContentFrameSinkPrivate::OnConnectionLost() {
  if (content_frame_sink_observer_)
    content_frame_sink_observer_->OnConnectionLost();
}

void ContentFrameSinkPrivate::OnObserverLost() {
  display_compositor_connection_->OnPrivateConnectionLost(
      compositor_frame_sink_id_);
}

}  // namespace cc
