// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/service/delegating_output_surface.h"

#include "base/bind.h"
#include "base/threading/thread_task_runner_handle.h"
#include "cc/output/compositor_frame.h"
#include "cc/surfaces/display.h"
#include "cc/surfaces/surface.h"
#include "cc/surfaces/surface_id_allocator.h"
#include "cc/surfaces/surface_manager.h"

namespace cc {

DelegatingOutputSurface::DelegatingOutputSurface(
    SurfaceManager* surface_manager,
    SurfaceIdAllocator* surface_id_allocator,
    Display* display,
    scoped_refptr<ContextProvider> context_provider,
    scoped_refptr<ContextProvider> worker_context_provider)
    : OutputSurface(std::move(context_provider),
                    std::move(worker_context_provider),
                    nullptr),
      surface_manager_(surface_manager),
      surface_id_allocator_(surface_id_allocator),
      display_(display),
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
#if 0
  gfx::Size frame_size =
      frame.delegated_frame_data->render_pass_list.back()->output_rect.size();
  if (frame_size.IsEmpty() || frame_size != last_swap_frame_size_) {
    if (!delegated_surface_id_.is_null()) {
      factory_.Destroy(delegated_surface_id_);
    }
    delegated_surface_id_ = //surface_id_allocator_->GenerateId();
    factory_.Create(delegated_surface_id_);
    last_swap_frame_size_ = frame_size;
  }
#endif
  if (delegated_surface_id_.is_null()) {
    delegated_surface_id_ =
        cc::SurfaceId(surface_id_allocator_->client_id(), 1, 1);
    factory_.Create(delegated_surface_id_);
  }
  if (display_) {
    display_->SetSurfaceId(delegated_surface_id_,
                           frame.metadata.device_scale_factor);
    gfx::Size frame_size =
        frame.delegated_frame_data->render_pass_list.back()->output_rect.size();
    display_->Resize(frame_size);
  }

  factory_.SubmitCompositorFrame(
      delegated_surface_id_, std::move(frame),
      base::Bind(&DelegatingOutputSurface::DidDrawCallback,
                 base::Unretained(this)));
}

bool DelegatingOutputSurface::BindToClient(OutputSurfaceClient* client) {
  DCHECK(thread_checker_.CalledOnValidThread());

  bool bind = OutputSurface::BindToClient(client);
  DCHECK(bind);

  // TODO(enne): this has to be after the bind, as it could cause a
  // SetBeginFrameSource which assumes a client_.  Probably need to
  // make SurfaceDisplayOutputSurface do this too.
  surface_manager_->RegisterSurfaceFactoryClient(
      surface_id_allocator_->client_id(), this);

  // Avoid initializing GL context here, as this should be sharing the
  // Display's context.
  if (display_) {
    display_->Initialize(this, surface_manager_,
                         surface_id_allocator_->client_id());
  }
  return true;
}

void DelegatingOutputSurface::ForceReclaimResources() {
  if (capabilities_.can_force_reclaim_resources &&
      !delegated_surface_id_.is_null()) {
    factory_.SubmitCompositorFrame(delegated_surface_id_, CompositorFrame(),
                                   SurfaceFactory::DrawCallback());
  }
}

void DelegatingOutputSurface::DetachFromClient() {
  DCHECK(HasClient());
  // Unregister the SurfaceFactoryClient here instead of the dtor so that only
  // one client is alive for this namespace at any given time.
  surface_manager_->UnregisterSurfaceFactoryClient(
      surface_id_allocator_->client_id());
  if (!delegated_surface_id_.is_null())
    factory_.Destroy(delegated_surface_id_);

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

void DelegatingOutputSurface::DidDrawCallback() {
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::Bind(&OutputSurfaceClient::DidSwapBuffersComplete,
                            base::Unretained(client_)));
  // // TODO(danakj): Why the lost check?
  // if (!output_surface_lost_)
  //   client_->DidSwapBuffersComplete();
}

}  // namespace cc
