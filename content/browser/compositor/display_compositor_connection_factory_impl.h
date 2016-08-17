// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_COMPOSITOR_DISPLAY_COMPOSITOR_CONNECTION_FACTORY_IMPL_H_
#define CONTENT_BROWSER_COMPOSITOR_DISPLAY_COMPOSITOR_CONNECTION_FACTORY_IMPL_H_

#include "cc/host/display_compositor_host.h"
#include "cc/ipc/compositor.mojom.h"

namespace content {

class DisplayCompositorConnectionFactoryImpl
    : public cc::DisplayCompositorConnectionFactory {
 public:
  DisplayCompositorConnectionFactoryImpl();

  // DisplayCompositorHost::Delegate implementation:
  cc::DisplayCompositorConnection* GetDisplayCompositorConnection() override;

 private:
  ~DisplayCompositorConnectionFactoryImpl() override;

  friend class base::RefCountedThreadSafe<
      DisplayCompositorConnectionFactoryImpl>;

  std::unique_ptr<cc::DisplayCompositorConnection> display_compositor_;

  DISALLOW_COPY_AND_ASSIGN(DisplayCompositorConnectionFactoryImpl);
};

}  // namespace content

#endif  // CONTENT_BROWSER_COMPOSITOR_DISPLAY_COMPOSITOR_CONNECTION_FACTORY_IMPL_H_
