// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_SERVICE_DISPLAY_OUTPUT_SURFACE_H_
#define CC_SERVICE_DISPLAY_OUTPUT_SURFACE_H_

#include "cc/output/output_surface.h"
#include "cc/service/service_export.h"

namespace cc {
class ServiceContextProvider;

class CC_SERVICE_EXPORT DisplayOutputSurface : public OutputSurface {
 public:
  explicit DisplayOutputSurface(
      scoped_refptr<ServiceContextProvider> context_provider);
  ~DisplayOutputSurface() override;

  // OutputSurface implementation.
  void SwapBuffers(CompositorFrame frame) override;
  uint32_t GetFramebufferCopyTextureFormat() override;
};

}  // namespace cc

#endif  // CC_SERVICE_DISPLAY_OUTPUT_SURFACE_H_
