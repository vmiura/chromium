#include "cc/service/service.h"

#include "base/auto_reset.h"
#include "base/memory/ptr_util.h"
#include "base/threading/thread_task_runner_handle.h"
#include "cc/animation/animation_host.h"
#include "cc/scheduler/compositor_timing_history.h"
#include "cc/scheduler/scheduler.h"
#include "cc/trees/layer_tree_host_impl.h"
#include "cc/trees/layer_tree_impl.h"

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
    owner_->scheduler()->DidLoseOutputSurface();
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
    return inside_draw_;
  }
  void RenewTreePriority() override {
    // TODO(hackathon): Default priority fine for now?
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
    owner_->compositor_client()->OnBeginMainFrame(args);
  }
  DrawResult ScheduledActionDrawAndSwapIfPossible() override {
    bool forced_draw = false;
    return DrawAndSwapInternal(forced_draw);
  }
  DrawResult ScheduledActionDrawAndSwapForced() override {
    bool forced_draw = false;
    return DrawAndSwapInternal(forced_draw);
  }
  void ScheduledActionCommit() override {
    owner_->host_impl()->BeginCommit();

    // TODO(hackathon): Need main side to commit.
    // blocked_main_commit().layer_tree_host->FinishCommitOnImplThread(
    //    owner_->host_impl());

    owner_->scheduler()->DidCommit();
    owner_->host_impl()->CommitComplete();
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
  DrawResult DrawAndSwapInternal(bool forced_draw) {
    base::AutoReset<bool> mark_inside(&inside_draw_, true);

    if (owner_->host_impl()->pending_tree()) {
      bool update_lcd_text = false;
      owner_->host_impl()->pending_tree()->UpdateDrawProperties(
          update_lcd_text);
    }

    // This method is called on a forced draw, regardless of whether we are able
    // to produce a frame, as the calling site on main thread is blocked until
    // its
    // request completes, and we signal completion here. If CanDraw() is false,
    // we
    // will indicate success=false to the caller, but we must still signal
    // completion to avoid deadlock.

    // We guard PrepareToDraw() with CanDraw() because it always returns a valid
    // frame, so can only be used when such a frame is possible. Since
    // DrawLayers() depends on the result of PrepareToDraw(), it is guarded on
    // CanDraw() as well.

    LayerTreeHostImpl::FrameData frame;
    bool draw_frame = false;

    DrawResult result;
    if (owner_->host_impl()->CanDraw()) {
      result = owner_->host_impl()->PrepareToDraw(&frame);
      draw_frame = forced_draw || result == DRAW_SUCCESS;
    } else {
      result = DRAW_ABORTED_CANT_DRAW;
    }

    if (draw_frame) {
      owner_->host_impl()->DrawLayers(&frame);
      result = DRAW_SUCCESS;
    } else {
      DCHECK_NE(DRAW_SUCCESS, result);
    }
    owner_->host_impl()->DidDrawAllLayers(frame);

    bool start_ready_animations = draw_frame;
    owner_->host_impl()->UpdateAnimationState(start_ready_animations);

    if (draw_frame) {
      if (owner_->host_impl()->SwapBuffers(frame))
        owner_->scheduler()->DidSwapBuffers();
    }

    // TODO(hackathon): tell main side?
    // channel_impl_->DidCommitAndDrawFrame();

    DCHECK_NE(INVALID_RESULT, result);
    return result;
  }

  Service* const owner_;
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;

  bool inside_draw_ = false;

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
  client_->SetNeedsCommitOnImplThread();
}

void Service::Commit(bool wait_for_activation, mojom::ContentFramePtr frame) {
  // TODO(piman): hackathon implement this
  NOTIMPLEMENTED();
}

}  // namespace cc
