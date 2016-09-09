// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/layers/surface_layer.h"

#include <stdint.h>

#include "base/macros.h"
#include "base/trace_event/trace_event.h"
#include "cc/ipc/content_frame.mojom.h"
#include "cc/layers/surface_layer_impl.h"
#include "cc/output/swap_promise.h"
#include "cc/trees/layer_tree_host.h"

namespace cc {

scoped_refptr<SurfaceLayer> SurfaceLayer::Create(
    const AddRefCallback& addref_callback,
    const ReleaseRefCallback& release_callback) {
  return make_scoped_refptr(
      new SurfaceLayer(addref_callback, release_callback));
}

SurfaceLayer::SurfaceLayer(const AddRefCallback& addref_callback,
                           const ReleaseRefCallback& release_callback)
    : surface_scale_(1.f),
      addref_callback_(addref_callback),
      release_callback_(release_callback) {}

SurfaceLayer::~SurfaceLayer() {
  DCHECK(!layer_tree_host());
}

void SurfaceLayer::WriteStructureMojom(
    const ContentFrameBuilderContext& context,
    cc::mojom::LayerStructure* mojom) {
  Layer::WriteStructureMojom(context, mojom);  // Before we override stuff.
  mojom->layer_type = cc::mojom::LayerType::SURFACE;
}

void SurfaceLayer::WritePropertiesMojom(
    const ContentFrameBuilderContext& context,
    cc::mojom::LayerProperties* mojom) {
  Layer::WritePropertiesMojom(context, mojom);
  mojom->surface_state = mojom::SurfaceLayerState::New();
  mojom::SurfaceLayerState* surface_state = mojom->surface_state.get();
  surface_state->id = surface_id_;
  surface_state->size = surface_size_;
  surface_state->scale = surface_scale_;
}

void SurfaceLayer::SetSurfaceId(const SurfaceId& surface_id,
                                float scale,
                                const gfx::Size& size) {
  if (layer_tree_host()) {
    // Add a ref for the new surface ID.
    addref_callback_.Run(surface_id);

    // Remove the ref for the old surface ID.
    release_callback_.Run(surface_id_);
  }

  surface_id_ = surface_id;
  surface_size_ = size;
  surface_scale_ = scale;

  UpdateDrawsContent(HasDrawableContent());
  SetNeedsPushProperties();
}

std::unique_ptr<LayerImpl> SurfaceLayer::CreateLayerImpl(
    LayerTreeImpl* tree_impl) {
  return SurfaceLayerImpl::Create(tree_impl, id());
}

bool SurfaceLayer::HasDrawableContent() const {
  return !surface_id_.is_null() && Layer::HasDrawableContent();
}

void SurfaceLayer::SetLayerTreeHost(LayerTreeHost* host) {
  if (layer_tree_host() == host) {
    Layer::SetLayerTreeHost(host);
    return;
  }

  if (!surface_id_.is_null()) {
    if (host)
      addref_callback_.Run(surface_id_);

    if (layer_tree_host())
      release_callback_.Run(surface_id_);
  }

  Layer::SetLayerTreeHost(host);
}

void SurfaceLayer::PushPropertiesTo(LayerImpl* layer) {
  Layer::PushPropertiesTo(layer);
  TRACE_EVENT0("cc", "SurfaceLayer::PushPropertiesTo");
  SurfaceLayerImpl* layer_impl = static_cast<SurfaceLayerImpl*>(layer);

  layer_impl->SetSurfaceId(surface_id_);
  layer_impl->SetSurfaceSize(surface_size_);
  layer_impl->SetSurfaceScale(surface_scale_);
}

}  // namespace cc
