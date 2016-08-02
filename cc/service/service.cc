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
  ClientImpl(Service* owner,
             const scoped_refptr<base::SingleThreadTaskRunner>& task_runner)
      : owner_(owner), task_runner_(task_runner) {}

  // LayerTreeHostImplClient implementation.
  void UpdateRendererCapabilitiesOnImplThread() override {
    // TODO(hackathon): back to main
  }
  void DidLoseOutputSurfaceOnImplThread() override {
    // TODO(hackathon): back to main
  }
  void CommitVSyncParameters(base::TimeTicks timebase,
                             base::TimeDelta interval) override {}
  void SetBeginFrameSource(BeginFrameSource* source) override {
    owner_->scheduler()->SetBeginFrameSource(source);
  }
  void SetEstimatedParentDrawTime(base::TimeDelta draw_time) override {
    owner_->scheduler()->SetEstimatedParentDrawTime(draw_time);
  }
  void DidSwapBuffersCompleteOnImplThread() override {
    owner_->scheduler()->DidSwapBuffersComplete();
    // TODO(hackathon): back to main
  }
  void OnCanDrawStateChanged(bool can_draw) override {
    owner_->scheduler()->SetCanDraw(can_draw);
  }
  void NotifyReadyToActivate() override {
    owner_->scheduler()->NotifyReadyToActivate();
  }
  void NotifyReadyToDraw() override {
    owner_->scheduler()->NotifyReadyToDraw();
  }
  void SetNeedsRedrawOnImplThread() override {
    owner_->scheduler()->SetNeedsRedraw();
  }
  void SetNeedsRedrawRectOnImplThread(const gfx::Rect& damage_rect) override {
    owner_->host_impl()->SetViewportDamage(damage_rect);
    SetNeedsRedrawOnImplThread();
  }
  void SetNeedsOneBeginImplFrameOnImplThread() override {
    owner_->scheduler()->SetNeedsOneBeginImplFrame();
  }
  void SetNeedsCommitOnImplThread() override {
    owner_->scheduler()->SetNeedsBeginMainFrame();
  }
  void SetNeedsPrepareTilesOnImplThread() override {
    owner_->scheduler()->SetNeedsPrepareTiles();
  }
  void SetVideoNeedsBeginFrames(bool needs_begin_frames) override {
    owner_->scheduler()->SetVideoNeedsBeginFrames(needs_begin_frames);
  }
  void PostAnimationEventsToMainThreadOnImplThread(
      std::unique_ptr<AnimationEvents> events) override {
    // TODO(hackathon): back to main
  }
  bool IsInsideDraw() override {
    // TODO(hackathon): implement.
    return false;
  }
  void RenewTreePriority() override {
    // TODO(hackathon): implement.
  }
  void PostDelayedAnimationTaskOnImplThread(const base::Closure& task,
                                            base::TimeDelta delay) override {
    task_runner_->PostDelayedTask(FROM_HERE, task, delay);
  }
  void DidActivateSyncTree() override {
    // TODO(hackathon): send sync ipc reply back to main (?)
  }
  void WillPrepareTiles() override {
    owner_->scheduler()->WillPrepareTiles();
  }
  void DidPrepareTiles() override {
    owner_->scheduler()->DidPrepareTiles();
  }
  void DidCompletePageScaleAnimationOnImplThread() override {
    // TODO(hackathon): back to main
  }
  void OnDrawForOutputSurface(bool resourceless_software_draw) override {
    NOTREACHED(); // webivew only
  }

  // SchedulerClient implementation.
  void WillBeginImplFrame(const BeginFrameArgs& args) override {
    owner_->host_impl()->WillBeginImplFrame(args);
  }
  void ScheduledActionSendBeginMainFrame(const BeginFrameArgs& args) override {
    // TODO(hackathon): back to main
  }
  DrawResult ScheduledActionDrawAndSwapIfPossible() override {
    // TODO(hackathon): implement.
    return DRAW_SUCCESS;
  }
  DrawResult ScheduledActionDrawAndSwapForced() override {
    // TODO(hackathon): implement.
    return DRAW_SUCCESS;
  }
  void ScheduledActionCommit() override {
    // TODO(hackathon): implement.
  }
  void ScheduledActionActivateSyncTree() override {
    owner_->host_impl()->ActivateSyncTree();
  }
  void ScheduledActionBeginOutputSurfaceCreation() override {
    owner_->CreateOutputSurface();
  }
  void ScheduledActionPrepareTiles() override {
    owner_->host_impl()->PrepareTiles();
  }
  void ScheduledActionInvalidateOutputSurface() override {
    NOTREACHED(); // webivew only
  }
  void DidFinishImplFrame() override {
    owner_->host_impl()->DidFinishImplFrame();
  }
  void SendBeginMainFrameNotExpectedSoon() override {
    // TODO(hackathon): back to main.
  }

 private:
  Service* const owner_;
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;

  DISALLOW_COPY_AND_ASSIGN(ClientImpl);
};

Service::Service(cc::mojom::CompositorRequest request,
                 cc::mojom::CompositorClientPtr client,
                 SharedBitmapManager* shared_bitmap_manager,
                 gpu::GpuMemoryBufferManager* gpu_memory_buffer_manager,
                 TaskGraphRunner* task_graph_runner)
    : client_(new ClientImpl(this, base::ThreadTaskRunnerHandle::Get())),
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
