// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/compositor/display_compositor_connection_factory_impl.h"

#include "cc/host/display_compositor_connection.h"
#include "content/browser/gpu/gpu_process_host.h"
#include "services/shell/public/cpp/interface_provider.h"

namespace content {

DisplayCompositorConnectionFactoryImpl::DisplayCompositorConnectionFactoryImpl()
    : main_task_runner_(base::ThreadTaskRunnerHandle::Get()) {}

void DisplayCompositorConnectionFactoryImpl::AddObserver(
    DisplayCompositorConnectionObserver* observer) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  observers_.AddObserver(observer);
}

void DisplayCompositorConnectionFactoryImpl::RemoveObserver(
    DisplayCompositorConnectionObserver* observer) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  observers_.RemoveObserver(observer);
}

cc::DisplayCompositorConnection*
DisplayCompositorConnectionFactoryImpl::GetDisplayCompositorConnection() {
  if (display_compositor_)
    return display_compositor_.get();

  cc::mojom::DisplayCompositorFactoryPtr display_compositor_factory;
  GpuProcessHost* host =
      GpuProcessHost::Get(GpuProcessHost::GPU_PROCESS_KIND_SANDBOXED,
                          CAUSE_FOR_GPU_LAUNCH_BROWSER_STARTUP);

  // Request a DisplayCompositorConnectionFactoryImpl interface from the GPU
  // process.
  host->GetRemoteInterfaces()->GetInterface(&display_compositor_factory);

  cc::mojom::DisplayCompositorClientPtr display_compositor_client;
  cc::mojom::DisplayCompositorClientRequest display_compositor_client_request =
      mojo::GetProxy(&display_compositor_client);
  cc::mojom::DisplayCompositorPtr display_compositor;
  cc::mojom::DisplayCompositorRequest display_compositor_request =
      mojo::GetProxy(&display_compositor);

  // Create a display compositor, passing MessagePipes in both directions.
  // Note: This should only be called once.
  display_compositor_factory->CreateDisplayCompositor(
      std::move(display_compositor_request),
      std::move(display_compositor_client));

  display_compositor_ = base::MakeUnique<cc::DisplayCompositorConnection>(
      std::move(display_compositor),
      std::move(display_compositor_client_request));

  display_compositor_->AddObserver(this);

  return display_compositor_.get();
}

DisplayCompositorConnectionFactoryImpl::
    ~DisplayCompositorConnectionFactoryImpl() {
  if (display_compositor_)
    display_compositor_->RemoveObserver(this);
}

void DisplayCompositorConnectionFactoryImpl::OnSurfaceCreated(
    const gfx::Size& frame_size,
    const cc::SurfaceId& surface_id) {
  if (!main_task_runner_->BelongsToCurrentThread()) {
    main_task_runner_->PostTask(
        FROM_HERE,
        base::Bind(&DisplayCompositorConnectionFactoryImpl::OnSurfaceCreated,
                   this, frame_size, surface_id));
    return;
  }
  FOR_EACH_OBSERVER(DisplayCompositorConnectionObserver, observers_,
                    OnSurfaceCreated(frame_size, surface_id));
}

}  // namespace content
