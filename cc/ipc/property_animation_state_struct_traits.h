// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_IPC_PROPERTY_ANIMATION_STATE_STRUCT_TRAITS_H_
#define CC_IPC_PROPERTY_ANIMATION_STATE_STRUCT_TRAITS_H_

#include "cc/animation/element_animations.h"
#include "cc/ipc/animation.mojom.h"

namespace mojo {

template <>
struct StructTraits<cc::mojom::PropertyAnimationState,
                    cc::ElementAnimations::PropertyAnimationState> {
  static bool currently_running_for_active_elements(
      const cc::ElementAnimations::PropertyAnimationState& s) {
    return s.currently_running_for_active_elements;
  }

  static bool currently_running_for_pending_elements(
      const cc::ElementAnimations::PropertyAnimationState& s) {
    return s.currently_running_for_pending_elements;
  }

  static bool potentially_animating_for_active_elements(
      const cc::ElementAnimations::PropertyAnimationState& s) {
    return s.potentially_animating_for_active_elements;
  }
  static bool potentially_animating_for_pending_elements(
      const cc::ElementAnimations::PropertyAnimationState& s) {
    return s.potentially_animating_for_pending_elements;
  }

  static bool Read(cc::mojom::PropertyAnimationStateDataView data,
                   cc::ElementAnimations::PropertyAnimationState* out) {
    out->currently_running_for_active_elements =
        data.currently_running_for_active_elements();
    out->currently_running_for_pending_elements =
        data.currently_running_for_pending_elements();
    out->potentially_animating_for_active_elements =
        data.potentially_animating_for_active_elements();
    out->potentially_animating_for_pending_elements =
        data.potentially_animating_for_pending_elements();

    return true;
  }
};

}  // namespace mojo

#endif  // CC_IPC_PROPERTY_ANIMATION_STATE_STRUCT_TRAITS_H_
