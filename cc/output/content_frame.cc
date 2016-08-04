// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/output/content_frame.h"

#include "cc/layers/layer_impl.h"
#include "cc/surfaces/surface_id.h"

#include "cc/trees/layer_tree_impl.h"

namespace cc {

ContentFrame::ContentFrame() {}

ContentFrame::ContentFrame(ContentFrame&& other) = default;

ContentFrame::~ContentFrame() {}

ContentFrame& ContentFrame::operator=(ContentFrame&& other) = default;

void ContentFrame::Set(LayerTreeImpl& layer_tree) {
  layer_list = layer_tree.layer_list();
  property_trees = layer_tree.property_trees();
  surface_layers =
      base::WrapUnique(new LayerImplList(layer_tree.SurfaceLayers()));
}

AggregatedContentFrame::AggregatedContentFrame() {}

AggregatedContentFrame::AggregatedContentFrame(AggregatedContentFrame&& other) =
    default;

AggregatedContentFrame::~AggregatedContentFrame() {}

AggregatedContentFrame& AggregatedContentFrame::operator=(
    AggregatedContentFrame&& other) = default;

}  // namespace cc
