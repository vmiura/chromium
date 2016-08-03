// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/playback/display_item_list_client_cache.h"

#include <utility>
#include <vector>

#include "third_party/skia/include/core/SkStream.h"

namespace cc {
namespace {

// Helper function to deserialize the content of |picture_data| into an
// DisplayItemList.
scoped_refptr<const DisplayItemList> DeserializeDisplayItemList(
    const DisplayItemListData& display_item_list_data) {
  SkMemoryStream stream(display_item_list_data.data);
  return DisplayItemList::CreateFromStream(&stream);
}

}  // namespace

DisplayItemListClientCache::DisplayItemListClientCache() = default;

DisplayItemListClientCache::~DisplayItemListClientCache() = default;

scoped_refptr<const DisplayItemList> DisplayItemListClientCache::GetDisplayItemList(
    uint32_t engine_picture_id) {
  return GetDisplayItemListFromCache(engine_picture_id);
}

void DisplayItemListClientCache::ApplyCacheUpdate(
    const std::vector<DisplayItemListData>& cache_update) {
  // Add new display_item_list from the |cache_update| to |display_item_list_|.
  for (const DisplayItemListData& display_item_list_data : cache_update) {
    DCHECK(display_item_lists_.find(display_item_list_data.unique_id) == display_item_lists_.end());
    scoped_refptr<const DisplayItemList> deserialized_display_item_list =
        DeserializeDisplayItemList(display_item_list_data);

    display_item_lists_[display_item_list_data.unique_id] = std::move(deserialized_display_item_list);

#if DCHECK_IS_ON()
    last_added_.insert(display_item_list_data.unique_id);
#endif  // DCHECK_IS_ON()
  }
}

void DisplayItemListClientCache::Flush() {
  // Calculate which pictures can now be removed. |added| is only used for
  // verifying that what we calculated matches the new items that have been
  // inserted into the cache.
  std::vector<uint32_t> added;
  std::vector<uint32_t> removed;
  reference_tracker_.CommitRefCounts(&added, &removed);

  VerifyCacheUpdateMatchesReferenceTrackerData(added);

  RemoveUnusedDisplayItemListsFromCache(removed);
  reference_tracker_.ClearRefCounts();
}

void DisplayItemListClientCache::MarkUsed(uint32_t engine_picture_id) {
  reference_tracker_.IncrementRefCount(engine_picture_id);
}

void DisplayItemListClientCache::RemoveUnusedDisplayItemListsFromCache(
    const std::vector<uint32_t>& removed) {
  for (const auto& display_item_list_id : removed) {
    auto entry = display_item_lists_.find(display_item_list_id);
    DCHECK(entry != display_item_lists_.end());
    display_item_lists_.erase(entry);
  }
}

scoped_refptr<const DisplayItemList> DisplayItemListClientCache::GetDisplayItemListFromCache(
    uint32_t display_item_list_id) {
  DCHECK(display_item_lists_.find(display_item_list_id) != display_item_lists_.end());
  return display_item_lists_[display_item_list_id];
}

void DisplayItemListClientCache::VerifyCacheUpdateMatchesReferenceTrackerData(
    const std::vector<uint32_t>& new_tracked_items) {
#if DCHECK_IS_ON()
  DCHECK_EQ(new_tracked_items.size(), last_added_.size());
  DCHECK(std::unordered_set<uint32_t>(new_tracked_items.begin(),
                                      new_tracked_items.end()) == last_added_);
  last_added_.clear();
#endif  // DCHECK_IS_ON()
}

}  // namespace cc
