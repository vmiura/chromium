// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/ipc/transform_operation_struct_traits.h"

namespace mojo {

// static
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

// static
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

cc::mojom::AnimationRunState
EnumTraits<cc::mojom::AnimationRunState, cc::Animation::RunState>::ToMojom(
    cc::Animation::RunState run_state) {
  switch (run_state) {
    case cc::Animation::WAITING_FOR_TARGET_AVAILABILITY:
      return cc::mojom::AnimationRunState::WAITING_FOR_TARGET_AVAILABILITY;
    case cc::Animation::WAITING_FOR_DELETION:
      return cc::mojom::AnimationRunState::WAITING_FOR_DELETION;
    case cc::Animation::STARTING:
      return cc::mojom::AnimationRunState::STARTING;
    case cc::Animation::RUNNING:
      return cc::mojom::AnimationRunState::RUNNING;
    case cc::Animation::PAUSED:
      return cc::mojom::AnimationRunState::PAUSED;
    case cc::Animation::FINISHED:
      return cc::mojom::AnimationRunState::FINISHED;
    case cc::Animation::ABORTED:
      return cc::mojom::AnimationRunState::ABORTED;
    case cc::Animation::ABORTED_BUT_NEEDS_COMPLETION:
      return cc::mojom::AnimationRunState::ABORTED_BUT_NEEDS_COMPLETION;
  }
  NOTREACHED();
  return cc::mojom::AnimationRunState::WAITING_FOR_TARGET_AVAILABILITY;
}

bool EnumTraits<cc::mojom::AnimationRunState, cc::Animation::RunState>::
    FromMojom(cc::mojom::AnimationRunState input,
              cc::Animation::RunState* out) {
  switch (input) {
    case cc::mojom::AnimationRunState::WAITING_FOR_TARGET_AVAILABILITY:
      *out = cc::Animation::WAITING_FOR_TARGET_AVAILABILITY;
      return true;
    case cc::mojom::AnimationRunState::WAITING_FOR_DELETION:
      *out = cc::Animation::WAITING_FOR_DELETION;
      return true;
    case cc::mojom::AnimationRunState::STARTING:
      *out = cc::Animation::STARTING;
      return true;
    case cc::mojom::AnimationRunState::RUNNING:
      *out = cc::Animation::RUNNING;
      return true;
    case cc::mojom::AnimationRunState::PAUSED:
      *out = cc::Animation::PAUSED;
      return true;
    case cc::mojom::AnimationRunState::FINISHED:
      *out = cc::Animation::FINISHED;
      return true;
    case cc::mojom::AnimationRunState::ABORTED:
      *out = cc::Animation::ABORTED;
      return true;
    case cc::mojom::AnimationRunState::ABORTED_BUT_NEEDS_COMPLETION:
      *out = cc::Animation::ABORTED_BUT_NEEDS_COMPLETION;
      return true;
  }
  NOTREACHED();
  *out = cc::Animation::WAITING_FOR_TARGET_AVAILABILITY;
  return false;
}

}  // namespace mojo
