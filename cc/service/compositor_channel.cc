// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/service/compositor_channel.h"

#include "cc/service/service.h"
#include "cc/service/service_factory.h"
#include "gpu/ipc/service/gpu_channel.h"

namespace cc {

CompositorChannel::CompositorChannel(
    cc::mojom::CompositorChannelRequest request,
    ServiceFactory* factory,
    scoped_refptr<base::SingleThreadTaskRunner> compositor_task_runner)
    : factory_(factory),
      compositor_task_runner_(compositor_task_runner),
      binding_(this, std::move(request)) {}

CompositorChannel::~CompositorChannel() = default;

void CompositorChannel::AddRefOnSurfaceId(const SurfaceId& id) {
  compositor_task_runner_->PostTask(
      FROM_HERE,
      base::Bind([](ServiceFactory* factory, const SurfaceId& id) {
          factory->surface_manager()->AddRefOnSurfaceId(id);
        },
        // If factory_ is destroyed, the compositor_task_runner_'s thread is
        // joined.
        factory_, id));
}

void CompositorChannel::MoveTempRefToRefOnSurfaceId(const SurfaceId& id) {
  compositor_task_runner_->PostTask(
      FROM_HERE,
      base::Bind([](ServiceFactory* factory, const SurfaceId& id) {
          factory->surface_manager()->MoveTempRefToRefOnSurfaceId(id);
        },
        // If factory_ is destroyed, the compositor_task_runner_'s thread is
        // joined.
        factory_, id));
}

}  // namespace cc
