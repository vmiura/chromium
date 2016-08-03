// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/playback/display_item_list_data.h"

namespace cc {

DisplayItemListData::DisplayItemListData(uint32_t unique_id, sk_sp<SkData> data)
    : unique_id(unique_id), data(data) {}

DisplayItemListData::DisplayItemListData(const DisplayItemListData& other) = default;

DisplayItemListData::~DisplayItemListData() = default;

}  // namespace cc
