#include "cc/service/service.h"

#include "base/auto_reset.h"
#include "base/memory/ptr_util.h"
#include "base/threading/thread_task_runner_handle.h"
#include "cc/animation/animation_host.h"
#include "cc/output/texture_mailbox_deleter.h"
#include "cc/scheduler/begin_frame_source.h"
#include "cc/scheduler/compositor_timing_history.h"
#include "cc/scheduler/delay_based_time_source.h"
#include "cc/scheduler/scheduler.h"
#include "cc/service/delegating_output_surface.h"
#include "cc/service/display_output_surface.h"
#include "cc/service/service_context_provider.h"
#include "cc/surfaces/display.h"
#include "cc/surfaces/display_scheduler.h"
#include "cc/trees/layer_tree_host_impl.h"
#include "cc/trees/layer_tree_impl.h"
#include "gpu/command_buffer/common/gles2_cmd_utils.h"
#include "gpu/command_buffer/client/shared_memory_limits.h"

// TODO(hackathon): For Hacks.
#include "cc/layers/solid_color_layer_impl.h"

namespace cc {

namespace {

LayerTreeSettings CreateLayerTreeSettings() {
  LayerTreeSettings settings;
  settings.use_output_surface_begin_frame_source = true;
  return settings;
}

}  // namespace

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
    owner_->compositor_client()->OnDidCompleteSwapBuffers();
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
    owner_->compositor_client()->OnDidCompletePageScaleAnimation();
  }
  void OnDrawForOutputSurface(bool resourceless_software_draw) override {
    NOTREACHED(); // webivew only
  }

  // SchedulerClient implementation.
  void WillBeginImplFrame(const BeginFrameArgs& args) override {
    owner_->host_impl()->WillBeginImplFrame(args);
  }
  void ScheduledActionSendBeginMainFrame(const BeginFrameArgs& args) override {
    owner_->compositor_client()->OnBeginMainFrame(0, args);
  }
  DrawResult ScheduledActionDrawAndSwapIfPossible() override {
    bool forced_draw = false;
    base::AutoReset<bool> mark_inside(&inside_draw_, true);
    return owner_->DrawAndSwap(forced_draw);
  }
  DrawResult ScheduledActionDrawAndSwapForced() override {
    bool forced_draw = false;
    base::AutoReset<bool> mark_inside(&inside_draw_, true);
    return owner_->DrawAndSwap(forced_draw);
  }
  void ScheduledActionCommit() override {
    owner_->host_impl()->BeginCommit();

    // TODO(hackathon): Need main side to commit.
    // blocked_main_commit().layer_tree_host->FinishCommitOnImplThread(
    //    owner_->host_impl());
    owner_->InsertHackyGreenLayer();

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
    owner_->compositor_client()->OnBeginMainFrameNotExpectedSoon();
  }

 private:
  Service* const owner_;
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
  bool inside_draw_ = false;

  DISALLOW_COPY_AND_ASSIGN(ClientImpl);
};

Service::Service(const gpu::SurfaceHandle& handle,
                 cc::mojom::CompositorRequest request,
                 cc::mojom::CompositorClientPtr client,
                 int id,
                 SharedBitmapManager* shared_bitmap_manager,
                 gpu::GpuMemoryBufferManager* gpu_memory_buffer_manager,
                 gpu::ImageFactory* image_factory,
                 SurfaceManager* surface_manager,
                 TaskGraphRunner* task_graph_runner)
    : task_runner_(base::ThreadTaskRunnerHandle::Get()),
      client_(new ClientImpl(this, task_runner_)),
      shared_bitmap_manager_(shared_bitmap_manager),
      gpu_memory_buffer_manager_(gpu_memory_buffer_manager),
      image_factory_(image_factory),
      surface_manager_(surface_manager),
      widget_(handle),
      surface_id_allocator_(id),
      scheduler_(client_.get(),
                 SchedulerSettings(),
                 id,
                 task_runner_.get(),
                 nullptr /* begin_frame_source */,
                 base::MakeUnique<CompositorTimingHistory>(
                     SchedulerSettings().using_synchronous_renderer_compositor,
                     CompositorTimingHistory::RENDERER_UMA,
                     &rendering_stats_)),
      task_runner_provider_(task_runner_, nullptr),
      its_the_impl_thread_i_promise_(&task_runner_provider_),
      main_thread_animation_host_lol_(AnimationHost::CreateMainInstance()),
      host_impl_(CreateLayerTreeSettings(),
                 client_.get(),
                 &task_runner_provider_,
                 &rendering_stats_,
                 shared_bitmap_manager,
                 gpu_memory_buffer_manager,
                 task_graph_runner,
                 main_thread_animation_host_lol_->CreateImplInstance(
                     false /* supports_impl_scrolling */),
                 id),
      compositor_client_(std::move(client)),
      binding_(this, std::move(request)) {
  LOG(ERROR) << "Service compositor " << this;
  surface_manager_->RegisterSurfaceClientId(surface_id_allocator_.client_id());
  scheduler_.SetVisible(true);
  compositor_client_->OnCompositorCreated();
}

Service::~Service() {
  LOG(ERROR) << "~Service compositor " << this;

  scheduler_.SetVisible(false);
  scheduler_.DidLoseOutputSurface();
  host_impl_.ReleaseOutputSurface();
  output_surface_.reset();

  surface_manager_->InvalidateSurfaceClientId(
      surface_id_allocator_.client_id());
}

void Service::CreateOutputSurface() {
  const bool root_compositor = widget_ != gfx::kNullAcceleratedWidget;
  LOG(ERROR) << "Service CreateOutputSurface for root: " << root_compositor;
  if (root_compositor) {
    auto begin_frame_source = base::MakeUnique<DelayBasedBeginFrameSource>(
        base::MakeUnique<DelayBasedTimeSource>(task_runner_.get()));

    scoped_refptr<ServiceContextProvider> display_context_provider(
        new ServiceContextProvider(widget_, gpu_memory_buffer_manager_,
                                   image_factory_, gpu::SharedMemoryLimits(),
                                   nullptr /* shared_context_provider */));

    auto output_surface = base::MakeUnique<DisplayOutputSurface>(
        std::move(display_context_provider));
    auto display_scheduler = base::MakeUnique<DisplayScheduler>(
        begin_frame_source.get(), task_runner_.get(),
        output_surface->capabilities().max_frames_pending);

    display_.reset(new Display(
        shared_bitmap_manager_,
        gpu_memory_buffer_manager_,
        // btw, cc_unittests pixel tests expect default RendeererSettings to
        // pass.
        CreateLayerTreeSettings().renderer_settings,
        std::move(begin_frame_source),
        std::move(output_surface),
        std::move(display_scheduler),
        base::MakeUnique<TextureMailboxDeleter>(task_runner_)));
  }

  scoped_refptr<ServiceContextProvider> compositor_context(
      new ServiceContextProvider(gfx::kNullAcceleratedWidget,
                                 gpu_memory_buffer_manager_, image_factory_,
                                 gpu::SharedMemoryLimits::ForMailboxContext(),
                                 nullptr));
  scoped_refptr<ServiceContextProvider> worker_context(
      new ServiceContextProvider(
          gfx::kNullAcceleratedWidget, gpu_memory_buffer_manager_,
          image_factory_, gpu::SharedMemoryLimits(), compositor_context.get()));

  // Keep the one in LTHI alive until InitializeRenderer() is complete.
  auto local_output_surface = std::move(output_surface_);

  output_surface_ = base::MakeUnique<DelegatingOutputSurface>(
      surface_manager_, &surface_id_allocator_,
      display_.get(),  // Will be null for non-root compositors.
      std::move(compositor_context), std::move(worker_context));
  host_impl_.InitializeRenderer(output_surface_.get());

  scheduler_.DidCreateAndInitializeOutputSurface();
}

DrawResult Service::DrawAndSwap(bool forced_draw) {
  if (host_impl_.pending_tree()) {
    bool update_lcd_text = false;
    host_impl_.pending_tree()->UpdateDrawProperties(update_lcd_text);
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
  if (host_impl_.CanDraw()) {
    result = host_impl_.PrepareToDraw(&frame);
    draw_frame = forced_draw || result == DRAW_SUCCESS;
  } else {
    result = DRAW_ABORTED_CANT_DRAW;
  }

  if (draw_frame) {
    host_impl_.DrawLayers(&frame);
    result = DRAW_SUCCESS;
  } else {
    DCHECK_NE(DRAW_SUCCESS, result);
  }
  host_impl_.DidDrawAllLayers(frame);

  bool start_ready_animations = draw_frame;
  host_impl_.UpdateAnimationState(start_ready_animations);

  if (draw_frame) {
    if (host_impl_.SwapBuffers(frame))
      scheduler_.DidSwapBuffers();
  }

  compositor_client_->OnDidCommitAndDrawFrame();

  DCHECK_NE(INVALID_RESULT, result);
  return result;
}

void Service::InsertHackyGreenLayer() {
  // TODO(hackathon): Some made up stuff, we should get these from a commit.
  host_impl_.SetViewportSize(gfx::Size(100, 100));
  auto* pending_tree = host_impl_.pending_tree();
  auto green_layer =
      SolidColorLayerImpl::Create(pending_tree, 1);
  green_layer->SetBackgroundColor(SK_ColorGREEN);
  green_layer->SetPosition(gfx::PointF(10, 10));
  green_layer->SetBounds(gfx::Size(80, 80));
  pending_tree->SetRootLayerForTesting(std::move(green_layer));
  pending_tree->BuildPropertyTreesForTesting();
}

void Service::SetNeedsBeginMainFrame() {
  client_->SetNeedsCommitOnImplThread();
}

void Service::Commit(bool wait_for_activation,
                     mojom::ContentFramePtr frame) {
  //DCHECK(!commit_completion_event_);
  //DCHECK(IsImplThread() && IsMainThreadBlocked());
  DCHECK(scheduler_.CommitPending());

  scheduler_.NotifyBeginMainFrameStarted(
      base::TimeTicks() /* TODO(hackathon): main_thread_start_time */);
  host_impl_.ReadyToCommit();

  // TODO(hackathon):
  //commit_completion_event_ = completion;
  //commit_completion_waits_for_activation_ = wait_for_activation;

  //DCHECK(!blocked_main_commit().layer_tree_host);
  //blocked_main_commit().layer_tree_host = layer_tree_host;
  scheduler_.NotifyReadyToCommit();

}

}  // namespace cc
