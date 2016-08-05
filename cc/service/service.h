#ifndef CC_SERVICE_SERVICE_H_
#define CC_SERVICE_SERVICE_H_

#include <memory>

#include "base/compiler_specific.h"
#include "base/memory/ref_counted.h"
#include "base/single_thread_task_runner.h"
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

class CC_SERVICE_EXPORT Service : public cc::mojom::Compositor {
 public:
  Service(const gpu::SurfaceHandle& handle,
          cc::mojom::CompositorRequest request,
          cc::mojom::CompositorClientPtr client,
          const cc::LayerTreeSettings& settings,
          int id,
          SharedBitmapManager* shared_bitmap_manager,
          gpu::GpuMemoryBufferManager* gpu_memory_buffer_manager,
          gpu::ImageFactory* image_factory,
          SurfaceManager* surface_manager,
          TaskGraphRunner* task_graph_runner);
  ~Service() override;

  // ClientImpl access.
  LayerTreeHostImpl* host_impl() { return &host_impl_; }
  Scheduler* scheduler() { return &scheduler_; }
  cc::mojom::CompositorClientPtr& compositor_client() { return compositor_client_; }
  void CreateOutputSurface();
  DrawResult DrawAndSwap(bool forced_draw);
  void DidActivateSyncTree();
  void ScheduledActionCommit();

  void InsertHackyGreenLayer();

  // cc::mojom::Compositor implementation.
  void RegisterChildCompositor(uint32_t client_id) override;
  void UnregisterChildCompositor(uint32_t client_id) override;
  void SatisfySequence(const SurfaceSequence& sequence) override;
  void SetNeedsBeginMainFrame() override;
  void SetNeedsRedraw(const gfx::Rect& damage_rect) override;
  void SetVisible(bool visible) override;
  void BeginMainFrameAborted(CommitEarlyOutReason reason) override;
  void Commit(bool wait_for_activation,
              mojom::ContentFramePtr frame,
              const CommitCallback& callback) override;
  void Destroy(const DestroyCallback& callback) override;

 private:
  void FinishCommit();
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;

  class ClientImpl;
  std::unique_ptr<ClientImpl> client_;

  class SetImplThread {
   public:
#if !DCHECK_IS_ON()
    explicit SetImplThread(TaskRunnerProvider* p) {}
#else
    explicit SetImplThread(TaskRunnerProvider* p) : p_(p) {
      p_->SetCurrentThreadIsImplThread(true);
    }

    ~SetImplThread() {
      p_->SetCurrentThreadIsImplThread(false);
    }

   private:
    TaskRunnerProvider* p_;
#endif
  };

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
  cc::mojom::CompositorClientPtr compositor_client_;
  mojo::StrongBinding<cc::mojom::Compositor> binding_;
  mojom::ContentFramePtr frame_for_commit_;
  bool wait_for_activation_ = false;
  CommitCallback commit_callback_;
  CommitCallback activation_callback_;

  // This is null for non-root service compositors.
  std::unique_ptr<Display> display_;

  // TODO(hackathon): Should be owned by LTHI but le sigh.
  std::unique_ptr<DelegatingOutputSurface> output_surface_;

  DISALLOW_COPY_AND_ASSIGN(Service);
};

}  // namespace cc

#endif  // CC_SERVICE_SERVICE_H_
