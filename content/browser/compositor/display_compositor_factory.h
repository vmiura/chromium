// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_COMPOSITOR_DISPLAY_COMPOSITOR_FACTORY_H_
#define CONTENT_BROWSER_COMPOSITOR_DISPLAY_COMPOSITOR_FACTORY_H_

#include "cc/host/display_compositor_host.h"
#include "cc/ipc/compositor.mojom.h"

namespace content {

// TODO(fsamuel): This should probably be content/browser/compositor not gpu.
class DisplayCompositorFactory : public cc::DisplayCompositorHost::Delegate {
 public:
  DisplayCompositorFactory();

  // DisplayCompositorHost::Delegate implementation:
  cc::DisplayCompositorConnection GetDisplayCompositorConnection() override;

 private:
  ~DisplayCompositorFactory() override;

  friend class base::RefCountedThreadSafe<DisplayCompositorFactory>;

  cc::mojom::DisplayCompositorFactoryPtr display_compositor_factory_;

  DISALLOW_COPY_AND_ASSIGN(DisplayCompositorFactory);
};

}  // namespace content

#endif  // CONTENT_BROWSER_COMPOSITOR_DISPLAY_COMPOSITOR_FACTORY_H_
