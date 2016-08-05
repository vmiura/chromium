// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/ipc/transform_operation_struct_traits.h"

namespace mojo {

cc::mojom::TargetProperty EnumTraits<cc::mojom::TargetProperty,
                                     cc::TargetProperty::Type>::ToMojom(
  cc::TargetProperty::Type target_property) {
   switch (target_property) {
     case cc::TargetProperty::TRANSFORM:
       return cc::mojom::TargetProperty::TRANSFORM;
     case cc::TargetProperty::OPACITY:
       return cc::mojom::TargetProperty::OPACITY;
     case cc::TargetProperty::FILTER:
       return cc::mojom::TargetProperty::FILTER;
     case cc::TargetProperty::SCROLL_OFFSET:
       return cc::mojom::TargetProperty::SCROLL_OFFSET;
     case cc::TargetProperty::BACKGROUND_COLOR:
       return cc::mojom::TargetProperty::BACKGROUND_COLOR;
   }
   NOTREACHED();
   return cc::mojom::TargetProperty::TRANSFORM;
}

bool EnumTraits<cc::mojom::TargetProperty, cc::TargetProperty::Type>::FromMojom(
    cc::mojom::TargetProperty input,
    cc::TargetProperty::Type* out) {
  switch (input) {
    case cc::mojom::TargetProperty::TRANSFORM:
      *out = cc::TargetProperty::TRANSFORM;
      return true;
    case cc::mojom::TargetProperty::OPACITY:
      *out = cc::TargetProperty::OPACITY;
      return true;
    case cc::mojom::TargetProperty::FILTER:
      *out = cc::TargetProperty::OPACITY;
      return true;
    case cc::mojom::TargetProperty::SCROLL_OFFSET:
      *out = cc::TargetProperty::SCROLL_OFFSET;
      return true;
    case cc::mojom::TargetProperty::BACKGROUND_COLOR:
      *out = cc::TargetProperty::BACKGROUND_COLOR;
      return true;
  }
  NOTREACHED();
  *out = cc::TargetProperty::TRANSFORM;
  return false;
}

}  // namespace mojo
