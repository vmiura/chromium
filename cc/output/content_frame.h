// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_OUTPUT_CONTENT_FRAME_H_
#define CC_OUTPUT_CONTENT_FRAME_H_

#include <memory>

#include "base/macros.h"
#include "cc/base/cc_export.h"
#include "cc/layers/layer_collections.h"
#include "cc/trees/property_tree.h"

namespace cc {
class SurfaceId;
class LayerImpl;
// HAckathon.
class LayerTreeImpl;

// ContentFrame contains LayerList and PropertyTrees.
class CC_EXPORT ContentFrame {
 public:
  ContentFrame();
  ContentFrame(ContentFrame&& other);
  ~ContentFrame();

  ContentFrame& operator=(ContentFrame&& other);

  // If not hackathon should take ownership.
  // OwnedLayerImplList layer_list;
  // std::unique_ptr<PropertyTrees> property_trees;
  // std::unique_ptr<LayerImplList> surface_layers;

  // HACKATHON TIME!
  void Set(LayerTreeImpl& layer_tree);
  OwnedLayerImplList* layer_list;
  PropertyTrees* property_trees;
  std::unique_ptr<LayerImplList> surface_layers;

 private:
  DISALLOW_COPY_AND_ASSIGN(ContentFrame);
};

// Like ContentFrame but no ownership of layers, can be passed directly into
// LayerTreeImpl.
class CC_EXPORT AggregatedContentFrame {
 public:
  AggregatedContentFrame();
  AggregatedContentFrame(AggregatedContentFrame&& other);
  ~AggregatedContentFrame();

  AggregatedContentFrame& operator=(AggregatedContentFrame&& other);

  LayerImplList layer_list;
  std::unique_ptr<PropertyTrees> property_trees;

 private:
  DISALLOW_COPY_AND_ASSIGN(AggregatedContentFrame);
};

}  // namespace cc

#endif  // CC_OUTPUT_CONTENT_FRAME_H_
