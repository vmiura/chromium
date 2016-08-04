// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_TREES_SERVICE_CONNECTION_H_
#define CC_TREES_SERVICE_CONNECTION_H_

#include "cc/ipc/compositor.mojom.h"
#include "cc/base/cc_export.h"
#include "mojo/public/cpp/bindings/interface_ptr.h"
#include "mojo/public/cpp/bindings/interface_request.h"


namespace cc {
namespace mojom {
class Compositor;
class CompositorClient;
}

struct CC_EXPORT ServiceConnection {
  ServiceConnection();
  ~ServiceConnection();

  mojo::InterfacePtr<cc::mojom::Compositor> compositor;
  mojo::InterfaceRequest<cc::mojom::CompositorClient> client_request;
};

}  // namespace cc

#endif  // CC_TREES_SERVICE_CONNECTION_H_
