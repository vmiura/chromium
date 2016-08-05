// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/service/delegating_output_surface.h"

#include "base/bind.h"
#include "base/threading/thread_task_runner_handle.h"
#include "cc/output/compositor_frame.h"
#include "cc/surfaces/display.h"
#include "cc/surfaces/surface.h"
#include "cc/surfaces/surface_manager.h"

namespace cc {

DelegatingOutputSurface::DelegatingOutputSurface(
    SurfaceManager* surface_manager,
    Display* display,
    uint32_t surface_client_id,
    scoped_refptr<ContextProvider> context_provider,
    scoped_refptr<ContextProvider> worker_context_provider)
    : OutputSurface(std::move(context_provider),
                    std::move(worker_context_provider),
                    nullptr),
      surface_manager_(surface_manager),
      display_(display),
      surface_client_id_(surface_client_id),
      factory_(surface_manager, this) {
  DCHECK(thread_checker_.CalledOnValidThread());
  capabilities_.delegated_rendering = true;
  capabilities_.adjust_deadline_for_parent = true;
  // TODO(hackathon): The LayerTreeHostImpl doesn't draw before the Display does
  // always, so no reclaim possible.
  capabilities_.can_force_reclaim_resources = false;

  // Display and DelegatingOutputSurface share a GL context, so sync
  // points aren't needed when passing resources between them.
  capabilities_.delegated_sync_points_required = false;
  factory_.set_needs_sync_points(false);
}

DelegatingOutputSurface::~DelegatingOutputSurface() {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (HasClient())
    DetachFromClient();
}

void DelegatingOutputSurface::SwapBuffers(CompositorFrame frame) {
  gfx::Size frame_size =
      frame.delegated_frame_data->render_pass_list.back()->output_rect.size();
  if (committed_surface_id_.is_null()) {
    DCHECK(frame_size == last_swap_frame_size_);
  } else {
    if (!swapped_surface_id_.is_null())
      factory_.Destroy(swapped_surface_id_);
    swapped_surface_id_ = committed_surface_id_;
    committed_surface_id_ = SurfaceId();
  }

  if (display_) {
    display_->SetSurfaceId(swapped_surface_id_,
                           frame.metadata.device_scale_factor);
    display_->Resize(frame_size);
  }

  factory_.SubmitCompositorFrame(
      swapped_surface_id_, std::move(frame),
      base::Bind(&DelegatingOutputSurface::DidDrawCallback,
                 base::Unretained(this), committed_surface_id_));
  last_swap_frame_size_ = frame_size;
}

bool DelegatingOutputSurface::BindToClient(OutputSurfaceClient* client) {
  DCHECK(thread_checker_.CalledOnValidThread());

  bool bind = OutputSurface::BindToClient(client);
  DCHECK(bind);

  // TODO(enne): this has to be after the bind, as it could cause a
  // SetBeginFrameSource which assumes a client_.  Probably need to
  // make SurfaceDisplayOutputSurface do this too.
  surface_manager_->RegisterSurfaceFactoryClient(
      surface_client_id_, this);

  // Avoid initializing GL context here, as this should be sharing the
  // Display's context.
  if (display_)
    display_->Initialize(this, surface_manager_, surface_client_id_);
  return true;
}

void DelegatingOutputSurface::ForceReclaimResources() {
  if (capabilities_.can_force_reclaim_resources &&
      !committed_surface_id_.is_null()) {
    factory_.SubmitCompositorFrame(committed_surface_id_, CompositorFrame(),
                                   SurfaceFactory::DrawCallback());
  }
}

void DelegatingOutputSurface::DetachFromClient() {
  DCHECK(HasClient());
  if (!swapped_surface_id_.is_null())
    factory_.Destroy(swapped_surface_id_);
  // Unregister the SurfaceFactoryClient here instead of the dtor so that only
  // one client is alive for this namespace at any given time.
  surface_manager_->UnregisterSurfaceFactoryClient(
      surface_client_id_);

  OutputSurface::DetachFromClient();
}

void DelegatingOutputSurface::BindFramebuffer() {
  // This is a delegating output surface, no framebuffer/direct drawing support.
  NOTREACHED();
}

uint32_t DelegatingOutputSurface::GetFramebufferCopyTextureFormat() {
  // This is a delegating output surface, no framebuffer/direct drawing support.
  NOTREACHED();
  return 0;
}

void DelegatingOutputSurface::ReturnResources(
    const ReturnedResourceArray& resources) {
  if (client_)
    client_->ReclaimResources(resources);
}

void DelegatingOutputSurface::SetBeginFrameSource(
    BeginFrameSource* begin_frame_source) {
  DCHECK(client_);
  client_->SetBeginFrameSource(begin_frame_source);
}

void DelegatingOutputSurface::DisplayOutputSurfaceLost() {
  output_surface_lost_ = true;
  DidLoseOutputSurface();
}

void DelegatingOutputSurface::DisplaySetMemoryPolicy(
    const ManagedMemoryPolicy& policy) {
  SetMemoryPolicy(policy);
}

void DelegatingOutputSurface::DidDrawCallback(const SurfaceId& surface_id) {
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::Bind(&OutputSurfaceClient::DidSwapBuffersComplete,
                            base::Unretained(client_), surface_id));
  // // TODO(danakj): Why the lost check?
  // if (!output_surface_lost_)
  //   client_->DidSwapBuffersComplete();
}

void DelegatingOutputSurface::SetDelegatedSurfaceId(const SurfaceId& id) {
  if (!committed_surface_id_.is_null())
    factory_.Destroy(committed_surface_id_);
  committed_surface_id_ = id;
  if (!id.is_null())
    factory_.Create(committed_surface_id_);
}

}  // namespace cc
