// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_PLAYBACK_DISPLAY_ITEM_LIST_CLIENT_CACHE_H_
#define CC_PLAYBACK_DISPLAY_ITEM_LIST_CLIENT_CACHE_H_

#include <stdint.h>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "cc/playback/display_item_list.h"
#include "cc/playback/display_item_list_data.h"
#include "cc/playback/reference_tracker.h"

namespace cc {

// TODO: Fix this.
// BlimpClientPictureCache provides functionality for caching SkPictures once
// they are received from the engine, and cleaning up once the pictures are no
// longer in use. It is required to update this cache when an SkPicture starts
// being used and when it is not longer in use by calling
// MarkPictureForRegistration and MarkPictureForUnregistration respectively.
class DisplayItemListClientCache {
 public:
  DisplayItemListClientCache();
  ~DisplayItemListClientCache();

  scoped_refptr<const DisplayItemList> GetDisplayItemList(uint32_t engine_picture_id);
  void ApplyCacheUpdate(
      const std::vector<DisplayItemListData>& cache_update);
  void Flush();
  void MarkUsed(uint32_t engine_picture_id);

 private:
  // Removes all pictures specified in |picture_ids| from |pictures_|. The
  // picture IDs passed to this function must refer to the pictures that are no
  // longer in use.
  void RemoveUnusedDisplayItemListsFromCache(const std::vector<uint32_t>& picture_ids);

  // Retrieves the DisplayItemList with the given |picture_id| from the cache. The
  // given |picture_id| is the unique ID that the engine used to identify the
  // picture in the DisplayItemListCacheUpdate that contained the image.
  scoped_refptr<const DisplayItemList> GetDisplayItemListFromCache(uint32_t display_item_list_id);

  // Verify that the incoming cache update matches the new items that were added
  // to the |reference_tracker_|.
  void VerifyCacheUpdateMatchesReferenceTrackerData(
      const std::vector<uint32_t>& new_tracked_items);

#if DCHECK_IS_ON()
  // A set of the items that were added when the last cache update was applied.
  // Used for verifying that the calculation from the registry matches the
  // expectations.
  std::unordered_set<uint32_t> last_added_;
#endif  // DCHECK_IS_ON()

  // The current cache of SkPictures. The key is the unique ID used by the
  // engine, and the value is the SkPicture itself.
  std::unordered_map<uint32_t, scoped_refptr<const DisplayItemList>> display_item_lists_;

  // The reference tracker maintains the reference count of used SkPictures.
  ReferenceTracker reference_tracker_;

  DISALLOW_COPY_AND_ASSIGN(DisplayItemListClientCache);
};

}  // namespace cc

#endif  // CC_PLAYBACK_DISPLAY_ITEM_LIST_CLIENT_CACHE_H_
