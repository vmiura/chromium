// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_RESOURCES_RESOURCE_FORMAT_UTILS_H_
#define CC_RESOURCES_RESOURCE_FORMAT_UTILS_H_

#include "cc/base/cc_transport_export.h"
#include "cc/resources/resource_format.h"
#include "third_party/skia/include/core/SkImageInfo.h"

namespace cc {

CC_TRANSPORT_EXPORT SkColorType
ResourceFormatToClosestSkColorType(ResourceFormat format);

}  // namespace cc

#endif  // CC_RESOURCES_RESOURCE_FORMAT_UTILS_H_
