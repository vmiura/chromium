// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_SERVICE_CONTENT_FRAME_SINK_H_
#define CC_SERVICE_CONTENT_FRAME_SINK_H_

#include <memory>

#include "base/compiler_specific.h"
#include "base/memory/ref_counted.h"
#include "base/single_thread_task_runner.h"
#include "cc/base/bulk_buffer_queue.h"
#include "cc/debug/rendering_stats_instrumentation.h"
#include "cc/ipc/compositor.mojom.h"
#include "cc/ipc/content_frame.mojom.h"
#include "cc/scheduler/scheduler.h"
#include "cc/service/service_export.h"
#include "cc/surfaces/surface_id_allocator.h"
#include "cc/trees/layer_tree_host_impl.h"
#include "cc/trees/task_runner_provider.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "ui/gfx/native_widget_types.h"

namespace gpu {
class GpuMemoryBufferManager;
class ImageFactory;
}

namespace cc {
class AnimationHost;
class Display;
class SharedBitmapManager;
class DelegatingOutputSurface;
class SurfaceManager;
class TaskGraphRunner;
class UIResourceRequest;

class CC_SERVICE_EXPORT ContentFrameSink : public cc::mojom::ContentFrameSink {
 public:
  ContentFrameSink(uint32_t client_id,
                   int32_t sink_id,
                   const gpu::SurfaceHandle& handle,
                   cc::mojom::ContentFrameSinkRequest request,
                   cc::mojom::ContentFrameSinkClientPtr client,
                   const cc::LayerTreeSettings& settings,
                   SharedBitmapManager* shared_bitmap_manager,
                   gpu::GpuMemoryBufferManager* gpu_memory_buffer_manager,
                   gpu::ImageFactory* image_factory,
                   SurfaceManager* surface_manager,
                   TaskGraphRunner* task_graph_runner);
  ~ContentFrameSink() override;

  // ClientImpl access.
  void ReleaseSurfaces();
  LayerTreeHostImpl* host_impl() { return &host_impl_; }
  Scheduler* scheduler() { return &scheduler_; }
  cc::mojom::ContentFrameSinkClientPtr& content_frame_sink_client() {
    return content_frame_sink_client_;
  }
  void CreateOutputSurface();
  DrawResult DrawAndSwap(bool forced_draw);
  void DidActivateSyncTree();
  void ScheduledActionCommit();

  // cc::mojom::ContentFrameSink implementation.
  void SetNeedsBeginMainFrame() override;
  void SetNeedsRedraw(const gfx::Rect& damage_rect) override;
  void SetVisible(bool visible) override;
  void BeginMainFrameAborted(CommitEarlyOutReason reason) override;
  void PrepareCommit(bool will_wait_for_activation,
                     mojom::ContentFramePtr frame) override;
  void PrepareCommitSync(bool will_wait_for_activation,
                         mojom::ContentFramePtr frame,
                         const PrepareCommitSyncCallback& callback) override;
  void WaitForActivation(const WaitForActivationCallback& callback) override;
  void DeleteBackings(const std::vector<uint32_t>& backings) override;
  void Destroy(const DestroyCallback& callback) override;

  void PrepareCommitInternal(bool will_wait_for_activation,
                             mojom::ContentFramePtr frame);

 private:
  class ClientImpl;

  class SetImplThread {
   public:
#if !DCHECK_IS_ON()
    explicit SetImplThread(TaskRunnerProvider* p) {}
#else
    explicit SetImplThread(TaskRunnerProvider* p) : p_(p) {
      p_->SetCurrentThreadIsImplThread(true);
    }

    ~SetImplThread() { p_->SetCurrentThreadIsImplThread(false); }

   private:
    TaskRunnerProvider* p_;
#endif
  };

  enum WaitForActivationState {
    kWaitForActivationNone,
    kWaitForActivationPrepared,
    kWaitForActivationCommitted,
    kWaitForActivationActivated,
  };

  void FinishCommit();

  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
  std::unique_ptr<ClientImpl> client_;

  SharedBitmapManager* const shared_bitmap_manager_;
  gpu::GpuMemoryBufferManager* const gpu_memory_buffer_manager_;
  gpu::ImageFactory* const image_factory_;
  SurfaceManager* const surface_manager_;

  gfx::AcceleratedWidget widget_ = gfx::kNullAcceleratedWidget;
  SurfaceIdAllocator surface_id_allocator_;
  SurfaceId surface_id_;

  RenderingStatsInstrumentation rendering_stats_;
  Scheduler scheduler_;
  TaskRunnerProvider task_runner_provider_;
  SetImplThread its_the_impl_thread_i_promise_;
  std::unique_ptr<AnimationHost> main_thread_animation_host_lol_;
  LayerTreeHostImpl host_impl_;
  BulkBufferReader bulk_buffer_reader_;
  cc::mojom::ContentFrameSinkClientPtr content_frame_sink_client_;
  mojo::StrongBinding<cc::mojom::ContentFrameSink> binding_;
  mojom::ContentFramePtr frame_for_commit_;
  WaitForActivationState wait_for_activation_state_ = kWaitForActivationNone;
  WaitForActivationCallback activation_callback_;
  std::vector<SurfaceId> released_surfaces_;

  // This is null for non-root service compositors.
  std::unique_ptr<Display> display_;

  // TODO(hackathon): Should be owned by LTHI but le sigh.
  std::unique_ptr<DelegatingOutputSurface> output_surface_;

  DISALLOW_COPY_AND_ASSIGN(ContentFrameSink);
};

}  // namespace cc

#endif  // CC_SERVICE_CONTENT_FRAME_SINK_H_
