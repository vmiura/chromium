// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_IPC_ELEMENT_ID_STRUCT_TRAITS_H_
#define CC_IPC_ELEMENT_ID_STRUCT_TRAITS_H_

#include "cc/ipc/element_id.mojom.h"
#include "cc/animation/element_id.h"

namespace mojo {

template <>
struct StructTraits<cc::mojom::ElementId, cc::ElementId> {
  static int primary_id(const cc::ElementId& id) {
    return id.primaryId;
  }

  static int secondary_id(const cc::ElementId& id) {
    return id.secondaryId;
  }

  static bool Read(cc::mojom::ElementIdDataView data,
                   cc::ElementId* out) {
    out->primaryId = data.primary_id();
    out->secondaryId = data.secondary_id();
    return true;
  }
};

}  // namespace

#endif  // CC_IPC_ELEMENT_ID_STRUCT_TRAITS_H_
