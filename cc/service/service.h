#ifndef CC_SERVICE_SERVICE_H_
#define CC_SERVICE_SERVICE_H_

#include <memory>

#include "base/compiler_specific.h"
#include "base/memory/ref_counted.h"
#include "base/single_thread_task_runner.h"
#include "cc/debug/rendering_stats_instrumentation.h"
#include "cc/ipc/compositor.mojom.h"
#include "cc/scheduler/scheduler.h"
#include "cc/service/service_export.h"
#include "cc/trees/layer_tree_host_impl.h"
#include "cc/trees/task_runner_provider.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace gpu {
class GpuMemoryBufferManager;
}

namespace cc {
class AnimationHost;
class SharedBitmapManager;
class TaskGraphRunner;

class CC_SERVICE_EXPORT Service : public cc::mojom::Compositor {
 public:
  Service(cc::mojom::CompositorRequest request,
          cc::mojom::CompositorClientPtr client,
          SharedBitmapManager* shared_bitmap_manager,
          gpu::GpuMemoryBufferManager* gpu_memory_buffer_manager,
          TaskGraphRunner* task_graph_runner);
  ~Service() override;

  void CreateOutputSurface();

  // cc::mojom::Compositor implementation.
  void SetNeedsBeginMainFrame() override;

  // ClientImpl access.
  LayerTreeHostImpl* host_impl() { return &host_impl_; }
  Scheduler* scheduler() { return &scheduler_; }

 private:
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

  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
  RenderingStatsInstrumentation rendering_stats_;
  Scheduler scheduler_;
  TaskRunnerProvider task_runner_provider_;
  SetImplThread its_the_impl_thread_i_promise_;
  std::unique_ptr<AnimationHost> main_thread_animation_host_lol_;
  LayerTreeHostImpl host_impl_;
  cc::mojom::CompositorClientPtr compositor_client_;
  mojo::StrongBinding<cc::mojom::Compositor> binding_;

  DISALLOW_COPY_AND_ASSIGN(Service);
};

}  // namespace cc

#endif  // CC_SERVICE_SERVICE_H_
