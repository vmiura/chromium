// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/client/compositor_proxy.h"

namespace cc {

CompositorProxy::CompositorProxy(
    cc::mojom::CompositorPtr compositor,
    cc::mojom::CompositorClientRequest compositor_client_request)
    : compositor_(std::move(compositor)),
      binding_(this, std::move(compositor_client_request)) {}

CompositorProxy::~CompositorProxy() = default;

void CompositorProxy::OnCompositorCreated() {
  fprintf(stderr, ">>>%s\n", __PRETTY_FUNCTION__);
}

}  // namespace cc
