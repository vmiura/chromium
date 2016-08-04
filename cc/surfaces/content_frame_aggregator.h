// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_SURFACES_CONTENT_FRAME_AGGREGATOR_H_
#define CC_SURFACES_CONTENT_FRAME_AGGREGATOR_H_

#include <memory>
#include <set>
#include <unordered_map>
#include <unordered_set>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "cc/layers/layer_collections.h"
#include "cc/resources/transferable_resource.h"
#include "cc/surfaces/surface_id.h"
#include "cc/surfaces/surfaces_export.h"

namespace cc {

class AggregatedContentFrame;
class ContentFrame;
class LayerImpl;
class ResourceProvider;
class Surface;
class SurfaceLayerImpl;
class SurfaceManager;

class CC_SURFACES_EXPORT ContentFrameAggregator {
 public:
  using SurfaceIndexMap = std::unordered_map<SurfaceId, int, SurfaceIdHash>;

  ContentFrameAggregator(SurfaceManager* manager);
  ~ContentFrameAggregator();

  AggregatedContentFrame Aggregate(const SurfaceId& surface_id);
  struct PropertyTreesState {
    int transform_tree_id;
    int clip_tree_id;
    int effect_tree_id;
    int scroll_tree_id;
  };

 private:
  void UpdatePropertyTreeState(const PropertyTreesState& state,
                               LayerImpl* layer);
  void AggregateInternal(const ContentFrame& input_frame,
                         int frame_index,
                         const SurfaceId& surface_id,
                         const LayerImpl* surface_layer);

  SurfaceManager* manager_;
  std::unordered_map<SurfaceId, PropertyTreesState, SurfaceIdHash>
      surface_property_tree_locator_;
  std::unordered_map<SurfaceId, size_t, SurfaceIdHash> surface_layer_locator_;
  std::unordered_map<SurfaceId, int, SurfaceIdHash> surface_drawn_frames_;

  AggregatedContentFrame* dest_content_frame_;
  LayerImplList surface_layers_;

  base::WeakPtrFactory<ContentFrameAggregator> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(ContentFrameAggregator);
};

}  // namespace cc

#endif  // CC_SURFACES_CONTENT_FRAME_AGGREGATOR_H_
