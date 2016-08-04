// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_SERVICE_CONTEXT_PROVIDER_H_
#define CC_SERVICE_CONTEXT_PROVIDER_H_

#include <stdint.h>

#include <memory>

#include "base/synchronization/lock.h"
#include "cc/output/context_provider.h"
#include "gpu/command_buffer/common/gles2_cmd_utils.h"
#include "ui/gfx/native_widget_types.h"

#include "gpu/ipc/common/surface_handle.h"

class GrContext;

namespace gpu {
class GLInProcessContext;
class GpuMemoryBufferManager;
class ImageFactory;
struct SharedMemoryLimits;
class SyncPointManager;
namespace gles2 {
class MailboxManager;
}
}

namespace skia_bindings {
class GrContextForGLES2Interface;
}

namespace cc {

class ServiceContextProvider : public ContextProvider {
 public:
  static void SetupThread(
    gpu::SyncPointManager* sync_point_manager,
    gpu::gles2::MailboxManager* mailbox_manager);
  explicit ServiceContextProvider(
      gpu::SurfaceHandle handle,
      gpu::GpuMemoryBufferManager* gpu_memory_buffer_manager,
      gpu::ImageFactory* image_factory,
      const gpu::SharedMemoryLimits& limits,
      ServiceContextProvider* shared_context);

  bool BindToCurrentThread() override;
  gpu::gles2::GLES2Interface* ContextGL() override;
  gpu::ContextSupport* ContextSupport() override;
  class GrContext* GrContext() override;
  void InvalidateGrContext(uint32_t state) override;
  base::Lock* GetLock() override;
  gpu::Capabilities ContextCapabilities() override;
  void DeleteCachedResources() override;
  void SetLostContextCallback(
      const LostContextCallback& lost_context_callback) override;

  uint32_t GetCopyTextureInternalFormat();

 protected:
  friend class base::RefCountedThreadSafe<ServiceContextProvider>;
  ~ServiceContextProvider() override;

 private:
  const gpu::gles2::ContextCreationAttribHelper attributes_;

  base::Lock context_lock_;
  std::unique_ptr<gpu::GLInProcessContext> context_;
  std::unique_ptr<skia_bindings::GrContextForGLES2Interface> gr_context_;
};

}  // namespace cc

#endif  // CC_SERVICE_CONTEXT_PROVIDER_H_
