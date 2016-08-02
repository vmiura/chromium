#include "cc/service/service.h"

#include "base/memory/ptr_util.h"
#include "base/threading/thread_task_runner_handle.h"
#include "cc/animation/animation_host.h"
#include "cc/scheduler/compositor_timing_history.h"
#include "cc/scheduler/scheduler.h"
#include "cc/trees/layer_tree_host_impl.h"

namespace cc {

class Service::ClientImpl : public LayerTreeHostImplClient,
                            public SchedulerClient {
 public:
  explicit ClientImpl(Service* owner) : owner_(owner) {}

  // LayerTreeHostImplClient implementation.
  void UpdateRendererCapabilitiesOnImplThread() override {}
  void DidLoseOutputSurfaceOnImplThread() override {}
  void CommitVSyncParameters(base::TimeTicks timebase,
                             base::TimeDelta interval) override {}
  void SetBeginFrameSource(BeginFrameSource* source) override {}
  void SetEstimatedParentDrawTime(base::TimeDelta draw_time) override {}
  void DidSwapBuffersCompleteOnImplThread() override {}
  void OnCanDrawStateChanged(bool can_draw) override {}
  void NotifyReadyToActivate() override {}
  void NotifyReadyToDraw() override {}
  // Please call these 3 functions through
  // LayerTreeHostImpl's SetNeedsRedraw(), SetNeedsRedrawRect() and
  // SetNeedsOneBeginImplFrame().
  void SetNeedsRedrawOnImplThread() override {}
  void SetNeedsRedrawRectOnImplThread(const gfx::Rect& damage_rect) override {}
  void SetNeedsOneBeginImplFrameOnImplThread() override {}
  void SetNeedsCommitOnImplThread() override {}
  void SetNeedsPrepareTilesOnImplThread() override {}
  void SetVideoNeedsBeginFrames(bool needs_begin_frames) override {}
  void PostAnimationEventsToMainThreadOnImplThread(
      std::unique_ptr<AnimationEvents> events) override {}
  bool IsInsideDraw() override { return false; }
  void RenewTreePriority() override {}
  void PostDelayedAnimationTaskOnImplThread(const base::Closure& task,
                                            base::TimeDelta delay) override {}
  void DidActivateSyncTree() override {}
  void WillPrepareTiles() override {}
  void DidPrepareTiles() override {}
  void DidCompletePageScaleAnimationOnImplThread() override {}
  void OnDrawForOutputSurface(bool resourceless_software_draw) override {}

  // SchedulerClient implementation.
  void WillBeginImplFrame(const BeginFrameArgs& args) override {}
  void ScheduledActionSendBeginMainFrame(const BeginFrameArgs& args) override {}
  DrawResult ScheduledActionDrawAndSwapIfPossible() override {
    return DRAW_SUCCESS;
  }
  DrawResult ScheduledActionDrawAndSwapForced() override {
    return DRAW_SUCCESS;
  }
  void ScheduledActionCommit() override {}
  void ScheduledActionActivateSyncTree() override {}
  void ScheduledActionBeginOutputSurfaceCreation() override {
    owner_->CreateOutputSurface();
  }
  void ScheduledActionPrepareTiles() override {}
  void ScheduledActionInvalidateOutputSurface() override {}
  void DidFinishImplFrame() override {}
  void SendBeginMainFrameNotExpectedSoon() override {}

 private:
  Service* owner_;

  DISALLOW_COPY_AND_ASSIGN(ClientImpl);
};

Service::Service(cc::mojom::CompositorRequest request,
                 cc::mojom::CompositorClientPtr client,
                 SharedBitmapManager* shared_bitmap_manager,
                 gpu::GpuMemoryBufferManager* gpu_memory_buffer_manager,
                 TaskGraphRunner* task_graph_runner)
    : client_(new ClientImpl(this)),
      task_runner_(base::ThreadTaskRunnerHandle::Get()),
      scheduler_(client_.get(),
                 SchedulerSettings(),
                 0 /* id */,
                 task_runner_.get(),
                 nullptr /* begin_frame_source */,
                 base::MakeUnique<CompositorTimingHistory>(
                     SchedulerSettings().using_synchronous_renderer_compositor,
                     CompositorTimingHistory::RENDERER_UMA,
                     &rendering_stats_)),
      task_runner_provider_(task_runner_, nullptr),
      its_the_impl_thread_i_promise_(&task_runner_provider_),
      main_thread_animation_host_lol_(AnimationHost::CreateMainInstance()),
      host_impl_(LayerTreeSettings(),
                 client_.get(),
                 &task_runner_provider_,
                 &rendering_stats_,
                 shared_bitmap_manager,
                 gpu_memory_buffer_manager,
                 task_graph_runner,
                 main_thread_animation_host_lol_->CreateImplInstance(
                     false /* supports_impl_scrolling */),
                 0 /* id */),
      compositor_client_(std::move(client)),
      binding_(this, std::move(request)) {
  LOG(ERROR) << "Service compositor " << this;
  compositor_client_->OnCompositorCreated();
}

Service::~Service() {
  LOG(ERROR) << "~Service compositor " << this;
}

void Service::CreateOutputSurface() {
  LOG(ERROR) << "Service CreateOutputSurface";
  // TODO(hackathon): Actually make a GL context and OutputSurface and give it
  // to LTHI::InitializeRenderer().
  scheduler_.DidCreateAndInitializeOutputSurface();
}

void Service::SetNeedsBeginMainFrame() {
  // TODO(hackathon): Implement this.
  cc::BeginFrameArgs args;
  compositor_client_->OnBeginMainFrame(args);
}

}  // namespace cc
