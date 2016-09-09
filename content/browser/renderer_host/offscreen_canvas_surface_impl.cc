// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/offscreen_canvas_surface_impl.h"

#include "base/bind_helpers.h"
#include "cc/surfaces/surface.h"
#include "cc/surfaces/surface_manager.h"
#include "content/browser/compositor/surface_utils.h"
#include "content/public/browser/browser_thread.h"

namespace content {

// static
void OffscreenCanvasSurfaceImpl::Create(
    mojo::InterfaceRequest<blink::mojom::OffscreenCanvasSurface> request) {
  // |binding_| will take ownership of OffscreenCanvasSurfaceImpl
  new OffscreenCanvasSurfaceImpl(std::move(request));
}

OffscreenCanvasSurfaceImpl::OffscreenCanvasSurfaceImpl(
    mojo::InterfaceRequest<blink::mojom::OffscreenCanvasSurface> request)
    : id_allocator_(
          new cc::SurfaceIdAllocator(AllocateCompositorFrameSinkId().client_id,
                                     0 /* sink_id */)),
      binding_(this, std::move(request)) {
  // GetSurfaceManager()->RegisterSurfaceClientId(id_allocator_->client_id());
}

OffscreenCanvasSurfaceImpl::~OffscreenCanvasSurfaceImpl() {
#if 0
  if (!GetSurfaceManager()) {
    // Inform both members that SurfaceManager's no longer alive to
    // avoid their destruction errors.
    if (surface_factory_)
        surface_factory_->DidDestroySurfaceManager();
  } else {
    GetSurfaceManager()->InvalidateSurfaceClientId(id_allocator_->client_id());
  }
  surface_factory_->Destroy(surface_id_);
#endif
}

void OffscreenCanvasSurfaceImpl::GetSurfaceId(
    const GetSurfaceIdCallback& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  surface_id_ = id_allocator_->GenerateId();

  callback.Run(surface_id_);
}

void OffscreenCanvasSurfaceImpl::RequestSurfaceCreation(
    const cc::SurfaceId& surface_id) {
#if 0
  cc::SurfaceManager* manager = GetSurfaceManager();
  if (!surface_factory_) {
    surface_factory_ = base::MakeUnique<cc::SurfaceFactory>(manager, this);
  }
  surface_factory_->Create(surface_id);
#endif
}

// TODO(619136): Implement cc::SurfaceFactoryClient functions for resources
// return.
void OffscreenCanvasSurfaceImpl::ReturnResources(
    const cc::ReturnedResourceArray& resources) {}

void OffscreenCanvasSurfaceImpl::WillDrawSurface(const cc::SurfaceId& id,
                                                 const gfx::Rect& damage_rect) {
}

void OffscreenCanvasSurfaceImpl::SetBeginFrameSource(
    cc::BeginFrameSource* begin_frame_source) {}

}  // namespace content
