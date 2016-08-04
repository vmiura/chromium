// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/surfaces/content_frame_aggregator.h"

#include <stddef.h>

#include <map>

#include "base/bind.h"
#include "base/containers/adapters.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/stl_util.h"
#include "base/trace_event/trace_event.h"
#include "cc/base/math_util.h"
#include "cc/layers/layer_impl.h"
#include "cc/layers/surface_layer_impl.h"
#include "cc/output/content_frame.h"
#include "cc/surfaces/surface.h"
#include "cc/surfaces/surface_factory.h"
#include "cc/surfaces/surface_manager.h"
#include "cc/trees/blocking_task_runner.h"
#include "cc/trees/clip_node.h"
#include "cc/trees/effect_node.h"
#include "cc/trees/property_tree.h"
#include "cc/trees/scroll_node.h"
#include "cc/trees/transform_node.h"

namespace cc {
namespace {
bool UnittestFrames(const AggregatedContentFrame& aggregated_frame,
                    const ContentFrame& content_frame) {
  bool success = true;
  success &=
      aggregated_frame.layer_list.size() == content_frame.layer_list->size();
  for (size_t id = 0; id < aggregated_frame.layer_list.size(); ++id) {
    success &= aggregated_frame.layer_list.at(id)->id() ==
               content_frame.layer_list->at(id)->id();
  }
  success &= aggregated_frame.property_trees->transform_tree.size() ==
             content_frame.property_trees->transform_tree.size();
  success &= aggregated_frame.property_trees->clip_tree.size() ==
             content_frame.property_trees->clip_tree.size();
  success &= aggregated_frame.property_trees->effect_tree.size() ==
             content_frame.property_trees->effect_tree.size();
  success &= aggregated_frame.property_trees->scroll_tree.size() ==
             content_frame.property_trees->scroll_tree.size();
  return success;
}

void UpdateTargetInfo(TransformTree* transform_tree,
                      int target_offset,
                      int content_target_offset) {
  transform_tree->SetTargetId(
      transform_tree->back()->id,
      transform_tree->TargetId(transform_tree->back()->id) + target_offset);
  transform_tree->SetContentTargetId(
      transform_tree->back()->id,
      transform_tree->ContentTargetId(transform_tree->back()->id) +
          content_target_offset);
}

template <typename T>
void UpdateNodeOwnerId(T* node, int owner_offset) {
  node->owner_id += owner_offset;
}

template <typename T>
void UpdateNodeId(T* node,
                  int offset,
                  ContentFrameAggregator::PropertyTreesState info) {}

void UpdateNodeId(EffectNode* node,
                  int offset,
                  ContentFrameAggregator::PropertyTreesState info) {
  node->target_id += offset;
  node->transform_id += info.transform_tree_id;
  node->clip_id += info.clip_tree_id;
}

void UpdateNodeId(ClipNode* node,
                  int offset,
                  ContentFrameAggregator::PropertyTreesState info) {
  node->transform_id += info.transform_tree_id;
  node->target_transform_id += info.transform_tree_id;
}

void UpdateNodeId(ScrollNode* node,
                  int offset,
                  ContentFrameAggregator::PropertyTreesState info) {
  node->transform_id += info.transform_tree_id;
}

template <typename T>
void CopyFromInputPropertyTree(
    const PropertyTree<T>& input_property_tree,
    int parent_surface_property_id,
    int owner_offset,
    int offset_in_output_tree,
    ContentFrameAggregator::PropertyTreesState extra_info,
    PropertyTree<T>* output_property_tree) {
  for (size_t id = 0; id < input_property_tree.size(); ++id) {
    const T& node = *input_property_tree.Node(id);
    if (node.id == PropertyTree<T>::kRootNodeId) {
      // PROBABLY DUMMY NOTES BECAUSE PROPERTY TREES ARE SILLY.
    } else if (node.id == PropertyTree<T>::kRootNodeId + 1) {
      output_property_tree->Insert(node, parent_surface_property_id);
    } else {
      output_property_tree->Insert(node,
                                   node.parent_id + offset_in_output_tree);
    }
    if (node.id != PropertyTree<T>::kRootNodeId) {
      UpdateNodeId(output_property_tree->back(), offset_in_output_tree,
                   extra_info);
      UpdateNodeOwnerId(output_property_tree->back(), owner_offset);
    }
  }
}

template <>
void CopyFromInputPropertyTree<TransformNode>(
    const PropertyTree<TransformNode>& input_property_tree,
    int parent_surface_property_id,
    int owner_offset,
    int offset_in_output_tree,
    ContentFrameAggregator::PropertyTreesState extra_info,
    PropertyTree<TransformNode>* output_property_tree) {
  TransformTree* output_transform_tree =
      static_cast<TransformTree*>(output_property_tree);
  for (size_t id = 0; id < input_property_tree.size(); ++id) {
    const TransformNode& node = *input_property_tree.Node(id);
    if (node.id == TransformTree::kRootNodeId) {
      // PROBABLY DUMMY NOTES BECAUSE PROPERTY TREES ARE SILLY.
    } else if (node.id == TransformTree::kRootNodeId + 1) {
      output_transform_tree->Insert(node, parent_surface_property_id);
      UpdateTargetInfo(output_transform_tree, parent_surface_property_id,
                       offset_in_output_tree);
    } else {
      output_transform_tree->Insert(node,
                                    node.parent_id + offset_in_output_tree);
      UpdateTargetInfo(output_transform_tree, offset_in_output_tree,
                       offset_in_output_tree);
    }
    if (node.id != TransformTree::kRootNodeId) {
      UpdateNodeOwnerId(output_property_tree->back(), owner_offset);
    }
  }
}

}  // namespace

ContentFrameAggregator::ContentFrameAggregator(SurfaceManager* manager)
    : manager_(manager),
      dest_content_frame_(new AggregatedContentFrame()),
      weak_factory_(this) {
  DCHECK(manager_);
}

ContentFrameAggregator::~ContentFrameAggregator() {}

void ContentFrameAggregator::UpdatePropertyTreeState(
    const PropertyTreesState& state,
    LayerImpl* layer) {
  int transform_tree_index = layer->transform_tree_index();
  int clip_tree_index = layer->clip_tree_index();
  int effect_tree_index = layer->effect_tree_index();
  int scroll_tree_index = layer->scroll_tree_index();

  layer->SetTransformTreeIndex(transform_tree_index + state.transform_tree_id);
  layer->SetClipTreeIndex(clip_tree_index + state.clip_tree_id);
  layer->SetEffectTreeIndex(effect_tree_index + state.effect_tree_id);
  layer->SetScrollTreeIndex(scroll_tree_index + state.scroll_tree_id);
}

void ContentFrameAggregator::AggregateInternal(const ContentFrame& input_frame,
                                               int frame_index,
                                               const SurfaceId& surface_id,
                                               const LayerImpl* surface_layer) {
  // Find correct conversion between current input_frame's id and destination
  // frame's id.
  size_t layer_list_size = dest_content_frame_->layer_list.size();
  size_t transform_tree_size =
      dest_content_frame_->property_trees->transform_tree.size();
  size_t clip_tree_size = dest_content_frame_->property_trees->clip_tree.size();
  size_t effect_tree_size =
      dest_content_frame_->property_trees->effect_tree.size();
  size_t scroll_tree_size =
      dest_content_frame_->property_trees->scroll_tree.size();
  surface_layer_locator_[surface_id] = static_cast<int>(layer_list_size);
  PropertyTreesState property_tree_size{
      static_cast<int>(transform_tree_size) - 1,
      static_cast<int>(clip_tree_size) - 1,
      static_cast<int>(effect_tree_size) - 1,
      static_cast<int>(scroll_tree_size) - 1};
  surface_property_tree_locator_[surface_id] = property_tree_size;

  CopyFromInputPropertyTree(
      input_frame.property_trees->transform_tree,
      surface_layer ? surface_layer->transform_tree_index() : -1,
      layer_list_size, property_tree_size.transform_tree_id, property_tree_size,
      &dest_content_frame_->property_trees->transform_tree);
  CopyFromInputPropertyTree(
      input_frame.property_trees->clip_tree,
      surface_layer ? surface_layer->clip_tree_index() : -1, layer_list_size,
      property_tree_size.clip_tree_id, property_tree_size,
      &dest_content_frame_->property_trees->clip_tree);
  CopyFromInputPropertyTree(
      input_frame.property_trees->effect_tree,
      surface_layer ? surface_layer->effect_tree_index() : -1, layer_list_size,
      property_tree_size.effect_tree_id, property_tree_size,
      &dest_content_frame_->property_trees->effect_tree);
  CopyFromInputPropertyTree(
      input_frame.property_trees->scroll_tree,
      surface_layer ? surface_layer->scroll_tree_index() : -1, layer_list_size,
      property_tree_size.scroll_tree_id, property_tree_size,
      &dest_content_frame_->property_trees->scroll_tree);

  LayerImplList layer_list;
  for (auto& layer : *input_frame.layer_list) {
    layer_list.push_back(layer.get());
  }

  if (frame_index != surface_drawn_frames_[surface_id]) {
    for (auto& layer : layer_list) {
      layer->UpdateLayerIdWithOffset(layer_list_size);
      UpdatePropertyTreeState(property_tree_size, layer);
    }
  }

  LayerImplList& dest_layer_list = dest_content_frame_->layer_list;
  auto replace_surface_layer_iter = std::find_if(
      surface_layers_.begin(), surface_layers_.end(),
      [surface_id](LayerImpl* layer) {
        return static_cast<SurfaceLayerImpl*>(layer)->surface_id() ==
               surface_id;
      });
  auto replace_point =
      replace_surface_layer_iter != surface_layers_.end()
          ? std::find(dest_layer_list.begin(), dest_layer_list.end(),
                      *replace_surface_layer_iter)
          : dest_layer_list.end();
  if (replace_point != dest_layer_list.end())
    dest_layer_list.erase(replace_point);
  dest_layer_list.insert(replace_point, layer_list.begin(), layer_list.end());
  surface_drawn_frames_[surface_id] = frame_index;

  surface_layers_.insert(surface_layers_.end(),
                         input_frame.surface_layers->begin(),
                         input_frame.surface_layers->end());

  // THE AGGREGATION IS A LIE!!
  // for (auto& layer : *input_frame.surface_layers) {
  //   SurfaceLayerImpl* surface_layer = static_cast<SurfaceLayerImpl*>(layer);
  //   Surface* surface =
  //   manager_->GetSurfaceForId(surface_layer->surface_id());
  //   if (surface)
  //     AggregateInternal(surface->GetContentFrame(), surface->frame_index(),
  //                       surface_layer->surface_id(), layer);
  // }
}

AggregatedContentFrame ContentFrameAggregator::Aggregate(
    const SurfaceId& root_surface_id) {
  Surface* surface = manager_->GetSurfaceForId(root_surface_id);
  AggregatedContentFrame current_frame;
  if (!surface)
    return current_frame;
  const ContentFrame& root_frame = surface->GetContentFrame();
  TRACE_EVENT0("cc", "ContentFrameAggregator::Aggregate");

  current_frame.property_trees = base::WrapUnique(new PropertyTrees);
  dest_content_frame_ = &current_frame;
  surface_layers_.clear();
  surface_layer_locator_.clear();
  surface_property_tree_locator_.clear();
  AggregateInternal(root_frame, surface->frame_index(), root_surface_id,
                    nullptr);

  DCHECK(UnittestFrames(current_frame, root_frame));
  return current_frame;
}

}  // namespace cc
