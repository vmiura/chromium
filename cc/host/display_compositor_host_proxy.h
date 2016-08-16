// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_HOST_DISPLAY_COMPOSITOR_HOST_PROXY_H_
#define CC_HOST_DISPLAY_COMPOSITOR_HOST_PROXY_H_

#include "cc/ipc/compositor.mojom.h"

namespace cc {

// TODO(fsamuel): Also make a DisplayCompositorClient
class DisplayCompositorHostProxy : public mojom::DisplayCompositorHost {
 public:
  DisplayCompositorHostProxy(int32_t process_id,
                             mojom::DisplayCompositorHostPtr host);

  ~DisplayCompositorHostProxy() override;

  // DisplayCompositorHost implementation.
  void RequestSurfaceManager(
      mojom::SurfaceManagerRequest surface_manager) override;
  void CreateContentFrameSink(int32_t routing_id,
                              mojom::LayerTreeSettingsPtr settings,
                              mojom::ContentFrameSinkRequest compositor,
                              mojom::ContentFrameSinkClientPtr client) override;

 private:
  mojom::DisplayCompositorHostPtr host_;
  DISALLOW_COPY_AND_ASSIGN(DisplayCompositorHostProxy);
};

}  // namespace cc

#endif  // CC_HOST_DISPLAY_COMPOSITOR_HOST_PROXY_H_
