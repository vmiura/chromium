// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/service/service_context_provider.h"

#include <stdint.h>

#include "base/lazy_instance.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "cc/resources/platform_color.h"
#include "gpu/GLES2/gl2extchromium.h"
#include "gpu/command_buffer/client/gl_in_process_context.h"
#include "gpu/command_buffer/client/gles2_implementation.h"
#include "gpu/command_buffer/client/gles2_lib.h"
#include "gpu/command_buffer/common/gles2_cmd_utils.h"
#include "gpu/command_buffer/service/in_process_command_buffer.h"
#include "gpu/command_buffer/service/framebuffer_completeness_cache.h"
#include "gpu/command_buffer/service/gpu_preferences.h"
#include "gpu/command_buffer/service/shader_translator_cache.h"
#include "gpu/command_buffer/service/sync_point_manager.h"
#include "gpu/skia_bindings/grcontext_for_gles2_interface.h"
#include "third_party/khronos/GLES2/gl2.h"
#include "third_party/khronos/GLES2/gl2ext.h"
#include "third_party/skia/include/gpu/GrContext.h"
#include "third_party/skia/include/gpu/gl/GrGLInterface.h"
#include "ui/gfx/native_widget_types.h"

namespace cc {

namespace {
class DeferredGpuCommandService;

base::LazyInstance<scoped_refptr<DeferredGpuCommandService>> g_service =
    LAZY_INSTANCE_INITIALIZER;

class DeferredGpuCommandService
    : public gpu::InProcessCommandBuffer::Service,
      public base::RefCountedThreadSafe<DeferredGpuCommandService> {
 public:
  static scoped_refptr<DeferredGpuCommandService> GetInstance() {
    if (!g_service.Get())
      g_service.Get() = make_scoped_refptr(new DeferredGpuCommandService);
    return g_service.Get();
  }

  // gpu::InProcessCommandBuffer::Service implementation.
  void ScheduleTask(const base::Closure& task) override {
    task_runner_->PostTask(FROM_HERE, task);
  }

  void ScheduleDelayedWork(const base::Closure& task) override {
    task_runner_->PostDelayedTask(FROM_HERE, task,
                                   base::TimeDelta::FromMilliseconds(2));
  }
  bool UseVirtualizedGLContexts() override { return true; }
  scoped_refptr<gpu::gles2::ShaderTranslatorCache> shader_translator_cache()
      override {
    if (!shader_translator_cache_) {
      shader_translator_cache_ = make_scoped_refptr(
          new gpu::gles2::ShaderTranslatorCache(gpu_preferences()));
    }
    return shader_translator_cache_;
  }
  scoped_refptr<gpu::gles2::FramebufferCompletenessCache>
  framebuffer_completeness_cache() override {
    if (!framebuffer_completeness_cache_.get()) {
      framebuffer_completeness_cache_ =
          make_scoped_refptr(new gpu::gles2::FramebufferCompletenessCache);
    }
    return framebuffer_completeness_cache_;
  }
  gpu::SyncPointManager* sync_point_manager() override {
    return &sync_point_manager_;
  }

  void AddRef() const override {
    base::RefCountedThreadSafe<DeferredGpuCommandService>::AddRef();
  }
  void Release() const override {
    base::RefCountedThreadSafe<DeferredGpuCommandService>::Release();
  }

 private:
  friend class base::RefCountedThreadSafe<DeferredGpuCommandService>;

  DeferredGpuCommandService()
      : task_runner_(base::ThreadTaskRunnerHandle::Get()), sync_point_manager_(true) {}

  ~DeferredGpuCommandService() override {
    base::AutoLock lock(tasks_lock_);
    DCHECK(tasks_.empty());
  }

  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;

  base::Lock tasks_lock_;
  std::queue<base::Closure> tasks_;

  gpu::SyncPointManager sync_point_manager_;
  scoped_refptr<gpu::gles2::ShaderTranslatorCache> shader_translator_cache_;
  scoped_refptr<gpu::gles2::FramebufferCompletenessCache>
      framebuffer_completeness_cache_;

  DISALLOW_COPY_AND_ASSIGN(DeferredGpuCommandService);
};

gpu::gles2::ContextCreationAttribHelper CreateAttributes() {
  gpu::gles2::ContextCreationAttribHelper attributes;
  attributes.alpha_size = -1;
  attributes.depth_size = 0;
  attributes.stencil_size = 0;
  attributes.samples = 0;
  attributes.sample_buffers = 0;
  attributes.fail_if_major_perf_caveat = false;
  attributes.bind_generates_resource = false;
  return attributes;
}

std::unique_ptr<gpu::GLInProcessContext> CreateTestInProcessContext(
    const gpu::gles2::ContextCreationAttribHelper& attributes,
    gfx::AcceleratedWidget widget,
    gpu::GpuMemoryBufferManager* gpu_memory_buffer_manager,
    gpu::ImageFactory* image_factory,
    const gpu::SharedMemoryLimits& limits,
    gpu::GLInProcessContext* shared_context) {
  const bool is_offscreen = widget == gfx::kNullAcceleratedWidget;
  return base::WrapUnique(gpu::GLInProcessContext::Create(
      DeferredGpuCommandService::GetInstance(), nullptr, is_offscreen, widget,
      shared_context, attributes, limits, gpu_memory_buffer_manager,
      image_factory));
}

}  // namespace

void ServiceContextProvider::SetupThread() {
  DeferredGpuCommandService::GetInstance();
}

ServiceContextProvider::ServiceContextProvider(
    gfx::AcceleratedWidget widget,
    gpu::GpuMemoryBufferManager* gpu_memory_buffer_manager,
    gpu::ImageFactory* image_factory,
    const gpu::SharedMemoryLimits& limits,
    ServiceContextProvider* shared_context)
    : attributes_(CreateAttributes()),
      context_(CreateTestInProcessContext(
          attributes_,
          widget,
          gpu_memory_buffer_manager,
          image_factory,
          limits,
          (shared_context ? shared_context->context_.get() : nullptr))) {}

ServiceContextProvider::~ServiceContextProvider() = default;

bool ServiceContextProvider::BindToCurrentThread() {
  return true;
}

gpu::gles2::GLES2Interface* ServiceContextProvider::ContextGL() {
  return context_->GetImplementation();
}

gpu::ContextSupport* ServiceContextProvider::ContextSupport() {
  return context_->GetImplementation();
}

class GrContext* ServiceContextProvider::GrContext() {
  if (gr_context_)
    return gr_context_->get();

  gr_context_.reset(new skia_bindings::GrContextForGLES2Interface(ContextGL()));
  return gr_context_->get();
}

void ServiceContextProvider::InvalidateGrContext(uint32_t state) {
  if (gr_context_)
    gr_context_->ResetContext(state);
}

base::Lock* ServiceContextProvider::GetLock() {
  return &context_lock_;
}

gpu::Capabilities ServiceContextProvider::ContextCapabilities() {
  gpu::Capabilities capabilities;
  capabilities.image = true;
  capabilities.texture_rectangle = true;
  capabilities.sync_query = true;
  switch (PlatformColor::Format()) {
    case PlatformColor::SOURCE_FORMAT_RGBA8:
      capabilities.texture_format_bgra8888 = false;
      break;
    case PlatformColor::SOURCE_FORMAT_BGRA8:
      capabilities.texture_format_bgra8888 = true;
      break;
  }
  return capabilities;
}

void ServiceContextProvider::DeleteCachedResources() {
  if (gr_context_)
    gr_context_->FreeGpuResources();
}

void ServiceContextProvider::SetLostContextCallback(
    const LostContextCallback& lost_context_callback) {
  // TODO(hackathon): Do want.
}

uint32_t ServiceContextProvider::GetCopyTextureInternalFormat() {
  if (attributes_.alpha_size > 0)
    return GL_RGBA;
  DCHECK_NE(attributes_.red_size, 0);
  DCHECK_NE(attributes_.green_size, 0);
  DCHECK_NE(attributes_.blue_size, 0);
  return GL_RGB;
}

}  // namespace cc
