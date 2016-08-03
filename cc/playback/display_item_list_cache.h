// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_PLAYBACK_DISPLAY_ITEM_LIST_CACHE_H_
#define CC_PLAYBACK_DISPLAY_ITEM_LIST_CACHE_H_

#include <memory>
#include <unordered_map>
#include <vector>

#include "base/macros.h"
#include "cc/playback/display_item_list_data.h"
#include "cc/playback/reference_tracker.h"

namespace cc {

class DisplayItemList;

// TODO: Update this comment!
// BlimpEnginePictureCache provides functionality for caching SkPictures before
// they are sent from the engine to the client. The cache is cleared after
// every time it is flushed which happens when CalculateCacheUpdateAndFlush()
// is called. The expected state of what the client already has cached is
// tracked. It is required to update this cache when an SkPicture
// starts being used and when it is not longer in use by calling
// MarkPictureForRegistration and MarkPictureForUnregistration respectively.
// The lifetime of a cache matches the lifetime of a specific compositor.
// All interaction with this class should happen on the main thread.
class DisplayItemListCache {
 public:
  DisplayItemListCache();
  ~DisplayItemListCache();

  void MarkUsed(const DisplayItemList* list);
  std::vector<DisplayItemListData> CalculateCacheUpdateAndFlush();

 private:
  // Serializes the DisplayItemList and adds it to |display_item_lists_|.
  void Put(const DisplayItemList* display_item_list);

  // The current cache of display item lists. Used for temporarily storing lists
  // until the next call to CalculateCacheUpdateAndFlush(), at which point this
  // map is cleared.
  std::unordered_map<uint32_t, DisplayItemListData> display_item_lists_;

  // The reference tracker maintains the reference count of used SkPictures.
  ReferenceTracker reference_tracker_;

  DISALLOW_COPY_AND_ASSIGN(DisplayItemListCache);
};

}  // namespace cc

#endif  // CC_PLAYBACK_DISPLAY_ITEM_LIST_CACHE_H_
