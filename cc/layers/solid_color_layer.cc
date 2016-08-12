// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/layers/solid_color_layer.h"

#include "cc/ipc/content_frame.mojom.h"
#include "cc/layers/solid_color_layer_impl.h"

namespace cc {

std::unique_ptr<LayerImpl> SolidColorLayer::CreateLayerImpl(
    LayerTreeImpl* tree_impl) {
  return SolidColorLayerImpl::Create(tree_impl, id());
}

scoped_refptr<SolidColorLayer> SolidColorLayer::Create() {
  return make_scoped_refptr(new SolidColorLayer());
}

SolidColorLayer::SolidColorLayer() {}

SolidColorLayer::~SolidColorLayer() {}

void SolidColorLayer::SetBackgroundColor(SkColor color) {
  SetContentsOpaque(SkColorGetA(color) == 255);
  Layer::SetBackgroundColor(color);
}

void SolidColorLayer::WriteStructureMojom(
    const ContentFrameBuilderContext& context,
    cc::mojom::LayerStructure* mojom) {
  Layer::WriteStructureMojom(context, mojom);  // Before we override stuff.
  mojom->layer_type = cc::mojom::LayerType::SOLID_COLOR;
}

}  // namespace cc
