// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_IPC_CONTENT_FRAME_ENUM_TRAITS_H_
#define CC_IPC_CONTENT_FRAME_ENUM_TRAITS_H_

#include "cc/ipc/content_frame.mojom.h"
#include "cc/scheduler/commit_earlyout_reason.h"

namespace mojo {

template <>
struct EnumTraits<cc::mojom::CommitEarlyOutReason, cc::CommitEarlyOutReason> {
  static cc::mojom::CommitEarlyOutReason ToMojom(
      cc::CommitEarlyOutReason reason) {
    switch (reason) {
      case cc::CommitEarlyOutReason::ABORTED_OUTPUT_SURFACE_LOST:
        // HACKATHON: Should be unused.
        NOTREACHED();
        return cc::mojom::CommitEarlyOutReason::ABORTED_OUTPUT_SURFACE_LOST;
      case cc::CommitEarlyOutReason::ABORTED_NOT_VISIBLE:
        return cc::mojom::CommitEarlyOutReason::ABORTED_NOT_VISIBLE;
      case cc::CommitEarlyOutReason::ABORTED_DEFERRED_COMMIT:
        return cc::mojom::CommitEarlyOutReason::ABORTED_DEFERRED_COMMIT;
      case cc::CommitEarlyOutReason::FINISHED_NO_UPDATES:
        return cc::mojom::CommitEarlyOutReason::FINISHED_NO_UPDATES;
    }
  }

  static bool FromMojom(cc::mojom::CommitEarlyOutReason mojom,
                        cc::CommitEarlyOutReason* reason) {
    switch (mojom) {
      case cc::mojom::CommitEarlyOutReason::ABORTED_OUTPUT_SURFACE_LOST:
        // HACKATHON: Should be unused.
        NOTREACHED();
        *reason = cc::CommitEarlyOutReason::ABORTED_OUTPUT_SURFACE_LOST;
        return true;
      case cc::mojom::CommitEarlyOutReason::ABORTED_NOT_VISIBLE:
        *reason = cc::CommitEarlyOutReason::ABORTED_NOT_VISIBLE;
        return true;
      case cc::mojom::CommitEarlyOutReason::ABORTED_DEFERRED_COMMIT:
        *reason = cc::CommitEarlyOutReason::ABORTED_DEFERRED_COMMIT;
        return true;
      case cc::mojom::CommitEarlyOutReason::FINISHED_NO_UPDATES:
        *reason = cc::CommitEarlyOutReason::FINISHED_NO_UPDATES;
        return true;
      default:
        return false;
    }
  }
};

}  // namespace mojo

#endif  // CC_IPC_CONTENT_FRAME_ENUM_TRAITS_H_
