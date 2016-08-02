// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_CLIENT_COMPOSITOR_PROXY_H_
#define CC_CLIENT_COMPOSITOR_PROXY_H_

#include "cc/client/client_export.h"
#include "cc/ipc/compositor.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace cc {

class CC_CLIENT_EXPORT CompositorProxy : public cc::mojom::CompositorClient {
 public:
  CompositorProxy(cc::mojom::CompositorPtr compositor,
                  cc::mojom::CompositorClientRequest compositor_client_request);
  ~CompositorProxy() override;

  // cc::mojom::CompositorClient implementation.
  void OnCompositorCreated() override;

 private:
  cc::mojom::CompositorPtr compositor_;
  mojo::Binding<cc::mojom::CompositorClient> binding_;
};

}  // namespace cc

#endif  // CC_CLIENT_COMPOSITOR_PROXY_H_
