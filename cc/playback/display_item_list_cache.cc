// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/playback/display_item_list_cache.h"

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "cc/playback/display_item_list.h"
#include "third_party/skia/include/core/SkStream.h"

namespace cc {

DisplayItemListCache::DisplayItemListCache() {}
DisplayItemListCache::~DisplayItemListCache() {}

void DisplayItemListCache::MarkUsed(const DisplayItemList* display_item_list) {
  DCHECK(display_item_list);
  reference_tracker_.IncrementRefCount(display_item_list->unique_id());

  // Do not serialize multiple times, even though the item is referred to from
  // multiple places.
  if (display_item_lists_.find(display_item_list->unique_id()) != display_item_lists_.end()) {
    return;
  }

  Put(display_item_list);
}

std::vector<DisplayItemListData>
DisplayItemListCache::CalculateCacheUpdateAndFlush() {
  std::vector<uint32_t> added;
  std::vector<uint32_t> removed;
  reference_tracker_.CommitRefCounts(&added, &removed);

  // Create cache update consisting of new display_item_lists.
  std::vector<DisplayItemListData> update;
  for (const uint32_t item : added) {
    auto entry = display_item_lists_.find(item);
    DCHECK(entry != display_item_lists_.end());
    update.push_back(entry->second);
  }

  // All new items will be sent to the client, so clear everything.
  display_item_lists_.clear();
  reference_tracker_.ClearRefCounts();

  return update;
}

void DisplayItemListCache::Put(const DisplayItemList* display_item_list) {
  // Store the display_item_list data until it is sent to the client.
  display_item_lists_.insert(
      std::make_pair(display_item_list->unique_id(),
                     DisplayItemListData(display_item_list->unique_id(),
                                         display_item_list->Serialize())));
}

}  // namespace cc
