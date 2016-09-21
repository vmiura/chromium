#include "cc/service/content_frame_sink.h"

#include <stack>

#include "base/auto_reset.h"
#include "base/debug/stack_trace.h"
#include "base/memory/ptr_util.h"
#include "base/threading/thread_task_runner_handle.h"
#include "cc/animation/animation_host.h"
#include "cc/ipc/content_frame.mojom.h"
#include "cc/ipc/layer.mojom.h"
#include "cc/layers/layer_impl.h"
#include "cc/layers/painted_scrollbar_layer_impl.h"
#include "cc/layers/picture_layer_impl.h"
#include "cc/layers/solid_color_layer_impl.h"
#include "cc/layers/surface_layer_impl.h"
#include "cc/output/renderer_capabilities.h"
#include "cc/output/renderer.h"
#include "cc/output/texture_mailbox_deleter.h"
#include "cc/output/content_frame.h"
#include "cc/resources/ui_resource_bitmap.h"
#include "cc/resources/ui_resource_request.h"
#include "cc/scheduler/begin_frame_source.h"
#include "cc/scheduler/compositor_timing_history.h"
#include "cc/scheduler/delay_based_time_source.h"
#include "cc/scheduler/scheduler.h"
#include "cc/service/delegating_output_surface.h"
#include "cc/service/display_compositor.h"
#include "cc/service/display_output_surface.h"
#include "cc/service/service_context_provider.h"
#include "cc/surfaces/display.h"
#include "cc/surfaces/display_scheduler.h"
#include "cc/trees/clip_node.h"
#include "cc/trees/effect_node.h"
#include "cc/trees/layer_tree_host_impl.h"
#include "cc/trees/layer_tree_impl.h"
#include "cc/trees/property_tree.h"
#include "cc/trees/scroll_node.h"
#include "cc/trees/transform_node.h"
#include "cc/trees/tree_synchronizer.h"
#include "gpu/command_buffer/common/gles2_cmd_utils.h"
#include "gpu/command_buffer/client/shared_memory_limits.h"
#include "mojo/public/cpp/system/platform_handle.h"

namespace cc {

class ContentFrameSink::ClientImpl : public LayerTreeHostImplClient,
                                     public SchedulerClient {
 public:
  ClientImpl(ContentFrameSink* owner,
             const scoped_refptr<base::SingleThreadTaskRunner>& task_runner)
      : owner_(owner), task_runner_(task_runner) {}

  // LayerTreeHostImplClient implementation.
  void UpdateRendererCapabilitiesOnImplThread() override {
    owner_->content_frame_sink_client()->OnRendererCapabilities(
        owner_->host_impl_.GetRendererCapabilities().MainThreadCapabilities());
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
  void DidSwapBuffersCompleteOnImplThread(
      const SurfaceId& surface_id) override {
    owner_->scheduler()->DidSwapBuffersComplete();
    owner_->content_frame_sink_client()->OnDidCompleteSwapBuffers(surface_id);
    owner_->ReleaseSurfaces();
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
  bool IsInsideDraw() override { return inside_draw_; }
  void RenewTreePriority() override {
    // TODO(hackathon): Default priority fine for now?
  }
  void PostDelayedAnimationTaskOnImplThread(const base::Closure& task,
                                            base::TimeDelta delay) override {
    task_runner_->PostDelayedTask(FROM_HERE, task, delay);
  }
  void DidActivateSyncTree() override { owner_->DidActivateSyncTree(); }
  void WillPrepareTiles() override { owner_->scheduler()->WillPrepareTiles(); }
  void DidPrepareTiles() override { owner_->scheduler()->DidPrepareTiles(); }
  void DidCompletePageScaleAnimationOnImplThread() override {
    owner_->content_frame_sink_client()->OnDidCompletePageScaleAnimation();
  }
  void OnDrawForOutputSurface(bool resourceless_software_draw) override {
    NOTREACHED();  // webivew only
  }

  // SchedulerClient implementation.
  void WillBeginImplFrame(const BeginFrameArgs& args) override {
    owner_->host_impl()->WillBeginImplFrame(args);
  }
  void ScheduledActionSendBeginMainFrame(const BeginFrameArgs& args) override {
    owner_->content_frame_sink_client()->OnBeginMainFrame(0, args);
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
  void ScheduledActionCommit() override { owner_->ScheduledActionCommit(); }
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
    NOTREACHED();  // webivew only
  }
  void DidFinishImplFrame() override {
    owner_->host_impl()->DidFinishImplFrame();
  }
  void SendBeginMainFrameNotExpectedSoon() override {
    owner_->content_frame_sink_client()->OnBeginMainFrameNotExpectedSoon();
  }

 private:
  ContentFrameSink* const owner_;
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
  bool inside_draw_ = false;

  DISALLOW_COPY_AND_ASSIGN(ClientImpl);
};

ContentFrameSink::ContentFrameSink(
    DisplayCompositor* display_compositor,
    uint32_t client_id,
    int32_t sink_id,
    const gpu::SurfaceHandle& handle,
    cc::mojom::ContentFrameSinkRequest request,
    cc::mojom::ContentFrameSinkPrivateRequest private_request,
    cc::mojom::ContentFrameSinkClientPtr client,
    const cc::LayerTreeSettings& settings,
    SharedBitmapManager* shared_bitmap_manager,
    gpu::GpuMemoryBufferManager* gpu_memory_buffer_manager,
    gpu::ImageFactory* image_factory,
    SurfaceManager* surface_manager,
    TaskGraphRunner* task_graph_runner)
    : display_compositor_(display_compositor),
      task_runner_(base::ThreadTaskRunnerHandle::Get()),
      client_(new ClientImpl(this, task_runner_)),
      shared_bitmap_manager_(shared_bitmap_manager),
      gpu_memory_buffer_manager_(gpu_memory_buffer_manager),
      image_factory_(image_factory),
      surface_manager_(surface_manager),
      widget_(handle),
      compositor_frame_sink_id_(client_id, sink_id),
      scheduler_(client_.get(),
                 settings.ToSchedulerSettings(),
                 client_id,
                 task_runner_.get(),
                 nullptr /* begin_frame_source */,
                 base::MakeUnique<CompositorTimingHistory>(
                     settings.using_synchronous_renderer_compositor,
                     CompositorTimingHistory::RENDERER_UMA,
                     &rendering_stats_)),
      task_runner_provider_(task_runner_, nullptr),
      its_the_impl_thread_i_promise_(&task_runner_provider_),
      main_thread_animation_host_lol_(AnimationHost::CreateMainInstance()),
      host_impl_(settings,
                 client_.get(),
                 &task_runner_provider_,
                 &rendering_stats_,
                 shared_bitmap_manager,
                 gpu_memory_buffer_manager,
                 task_graph_runner,
                 main_thread_animation_host_lol_->CreateImplInstance(
                     false /* supports_impl_scrolling */),
                 client_id,
                 std::unique_ptr<ImageDecodeProxy>(
                     new ImageDecodeProxy(client.get()))),
      bulk_buffer_reader_(BulkBufferWriter::kDefaultBackingSize),
      content_frame_sink_client_(std::move(client)),
      binding_(this, std::move(request)),
      private_binding_(this, std::move(private_request)),
      weak_factory_(this) {
  const bool root_compositor = widget_ != gfx::kNullAcceleratedWidget;
  LOG(ERROR) << "ContentFrameSink[" << this << "] is root " << root_compositor
             << " client_id: " << compositor_frame_sink_id_.client_id
             << " sink_id: " << compositor_frame_sink_id_.sink_id;
  content_frame_sink_client_->OnCompositorCreated(compositor_frame_sink_id_);
  binding_.set_connection_error_handler(base::Bind(
      &ContentFrameSink::OnClientConnectionLost, weak_factory_.GetWeakPtr()));
  private_binding_.set_connection_error_handler(base::Bind(
      &ContentFrameSink::OnPrivateConnectionLost, weak_factory_.GetWeakPtr()));
}

ContentFrameSink::~ContentFrameSink() {
  LOG(ERROR) << "~ContentFrameSink" << this;

  ReleaseSurfaces();
  scheduler_.SetVisible(false);
  scheduler_.DidLoseOutputSurface();
  host_impl_.ReleaseOutputSurface();
  output_surface_.reset();
  client_.reset();

  surface_manager_->RemoveRefOnSurfaceId(surface_id_);
  // If this is a top level ContentFrameSink then we also take the ownership of
  // the surface from the display compositor host.
  if (widget_ != gfx::kNullAcceleratedWidget)
    surface_manager_->RemoveRefOnSurfaceId(surface_id_);
}

void ContentFrameSink::ReleaseSurfaces() {
  for (const SurfaceId& surface_id : released_surfaces_) {
    auto it = surface_refs_.find(surface_id);
    CHECK((it != surface_refs_.end()) && (it->second > 0));
    it->second--;
    fprintf(stderr, ">>>%s surface_id: %s\n", __PRETTY_FUNCTION__,
            surface_id.ToString().c_str());
    surface_manager_->RemoveRefOnSurfaceId(surface_id);
  }
  released_surfaces_.clear();
}

void ContentFrameSink::CreateOutputSurface() {
  if (output_surface_ && !surface_id_.is_null())
    output_surface_->SetDelegatedSurfaceId(SurfaceId());

  const bool root_compositor = widget_ != gfx::kNullAcceleratedWidget;
  if (root_compositor) {
    auto begin_frame_source = base::MakeUnique<DelayBasedBeginFrameSource>(
        base::MakeUnique<DelayBasedTimeSource>(task_runner_.get()));

    scoped_refptr<ServiceContextProvider> display_context_provider(
        new ServiceContextProvider(widget_, gpu_memory_buffer_manager_,
                                   image_factory_, gpu::SharedMemoryLimits(),
                                   nullptr /* shared_context_provider */));
    if (!display_context_provider->BindToCurrentThread())
      return;

    auto output_surface = base::MakeUnique<DisplayOutputSurface>(
        std::move(display_context_provider));
    auto display_scheduler = base::MakeUnique<DisplayScheduler>(
        begin_frame_source.get(), task_runner_.get(),
        output_surface->capabilities().max_frames_pending);

    display_.reset(new Display(
        shared_bitmap_manager_, gpu_memory_buffer_manager_,
        // btw, cc_unittests pixel tests expect default RendererSettings to
        // pass.
        host_impl_.settings().renderer_settings, std::move(begin_frame_source),
        std::move(output_surface), std::move(display_scheduler),
        base::MakeUnique<TextureMailboxDeleter>(task_runner_)));
  }

  scoped_refptr<ServiceContextProvider> compositor_context(
      new ServiceContextProvider(gfx::kNullAcceleratedWidget,
                                 gpu_memory_buffer_manager_, image_factory_,
                                 gpu::SharedMemoryLimits::ForMailboxContext(),
                                 nullptr));
  if (!compositor_context->BindToCurrentThread())
    return;
  scoped_refptr<ServiceContextProvider> worker_context(
      new ServiceContextProvider(
          gfx::kNullAcceleratedWidget, gpu_memory_buffer_manager_,
          image_factory_, gpu::SharedMemoryLimits(), compositor_context.get()));
  if (!worker_context->BindToCurrentThread())
    return;

  // Keep the one in LTHI alive until InitializeRenderer() is complete.
  auto local_output_surface = std::move(output_surface_);

  output_surface_ = base::MakeUnique<DelegatingOutputSurface>(
      surface_manager_,
      display_.get(),  // Will be null for non-root compositors.
      compositor_frame_sink_id_, std::move(compositor_context),
      std::move(worker_context));
  host_impl_.InitializeRenderer(output_surface_.get());

  if (!surface_id_.is_null())
    output_surface_->SetDelegatedSurfaceId(surface_id_);

  scheduler_.DidCreateAndInitializeOutputSurface();
}

DrawResult ContentFrameSink::DrawAndSwap(bool forced_draw) {
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

  content_frame_sink_client_->OnDidCommitAndDrawFrame();

  DCHECK_NE(INVALID_RESULT, result);
  return result;
}

void ContentFrameSink::AddRefOnSurfaceId(const SurfaceId& id) {
  surface_refs_[id]++;
  surface_manager_->AddRefOnSurfaceId(id);
}

void ContentFrameSink::TransferRef(const SurfaceId& id) {
  // SurfaceManager doesn't know about scoping of refs.
  // This is purely a book keeping operation. We trust that
  // the display compositor host is doing the right thing in
  // this case.
  surface_refs_[id]++;
}

void ContentFrameSink::RegisterChildSink(
    const CompositorFrameSinkId& child_client_id) {
  fprintf(stderr, ">>>%s child_client_id: %s\n", __PRETTY_FUNCTION__,
          child_client_id.ToString().c_str());
  surface_manager_->RegisterSurfaceNamespaceHierarchy(compositor_frame_sink_id_,
                                                      child_client_id);
}

void ContentFrameSink::UnregisterChildSink(
    const CompositorFrameSinkId& child_client_id) {
  surface_manager_->UnregisterSurfaceNamespaceHierarchy(
      compositor_frame_sink_id_, child_client_id);
}

void ContentFrameSink::SetNeedsBeginMainFrame() {
  TRACE_EVENT0("cc", "ContentFrameSink::SetNeedsBeginFrame");
  client_->SetNeedsCommitOnImplThread();
}

void ContentFrameSink::SetNeedsRedraw(const gfx::Rect& damage_rect) {
  TRACE_EVENT0("cc", "ContentFrameSink::SetNeedsRedraw");
  host_impl_.SetViewportDamage(damage_rect);
  scheduler_.SetNeedsRedraw();
}

void ContentFrameSink::SetVisible(bool visible) {
  TRACE_EVENT0("cc", "ContentFrameSink::SetVisible");
  host_impl_.SetVisible(visible);
  scheduler_.SetVisible(visible);
}

void ContentFrameSink::PrepareCommit(const SurfaceId& surface_id,
                                     bool will_wait_for_activation,
                                     mojom::ContentFramePtr frame) {
  TRACE_EVENT1("cc", "ContentFrameSink::PrepareCommit", "wait_for_activation",
               will_wait_for_activation);
  if (surface_id_ == surface_id) {
    DCHECK_EQ(wait_for_activation_state_, kWaitForActivationNone);
    DCHECK(!frame_for_commit_);
    DCHECK(scheduler_.CommitPending());

    bool viewport_changed_size =
        frame->device_viewport_size != host_impl_.device_viewport_size();
    // TODO(hackathon): AND !device scale factor changed.
    DCHECK(!viewport_changed_size);
    // First commit should always include a new surface ID.
    DCHECK(!surface_id_.is_null());
  } else {
    bool viewport_changed_size =
        frame->device_viewport_size != host_impl_.device_viewport_size();
    LayerTreeImpl* last_commit_tree = host_impl_.pending_tree();
    if (!last_commit_tree)
      last_commit_tree = host_impl_.active_tree();
    bool dsf_changed =
        frame->device_scale_factor != last_commit_tree->device_scale_factor();
    DCHECK(surface_id_.is_null() || viewport_changed_size || dsf_changed);

    // This ref belongs to the ContentFrameSink until it receives a new surface
    // ID.
    surface_manager_->AddRefOnSurfaceId(surface_id);
    // This ref belongs to the display compositor host.
    surface_manager_->AddRefOnSurfaceId(surface_id);
    if (!surface_id_.is_null())
      surface_manager_->RemoveRefOnSurfaceId(surface_id_);
    // If this is a top level ContentFrameSink then we also take the ownership
    // of the surface from the display compositor host.
    if (widget_ != gfx::kNullAcceleratedWidget)
      surface_manager_->RemoveRefOnSurfaceId(surface_id_);
    surface_id_ = surface_id;
    if (output_surface_)
      output_surface_->SetDelegatedSurfaceId(surface_id_);
    LOG(ERROR) << "New SurfaceID " << surface_id_.ToString();
  }

  PrepareCommitInternal(will_wait_for_activation, std::move(frame));
}

void ContentFrameSink::ValidateAndRecordReleasedSurfaces(
    const std::vector<SurfaceId>& released_surfaces) {
  if (released_surfaces.empty())
    return;
  for (const SurfaceId& surface_id : released_surfaces) {
    auto it = surface_refs_.find(surface_id);
    if (it == surface_refs_.end()) {
      // This can happen if the display compositor service has restarted and
      // these are IDs that were generated in the previous version of the
      // display compositor.
      continue;
    }
    CHECK(it->second > 0)
        << "Trying to release a surface that the client doesn't own a "
           "reference to.";
    released_surfaces_.push_back(surface_id);
  }
}

void ContentFrameSink::PrepareCommitInternal(bool will_wait_for_activation,
                                             mojom::ContentFramePtr frame) {
  ValidateAndRecordReleasedSurfaces(frame->released_surfaces);

  {
    TRACE_EVENT1("cc", "BulkBufferBackingHandle import", "count",
                 frame->new_handles.size());
    std::vector<BulkBufferBackingHandle> new_handles;
    new_handles.reserve(frame->new_handles.size());
    for (auto& new_handle : frame->new_handles) {
      base::PlatformFile platform_file;
      MojoResult unwrap_result = mojo::UnwrapPlatformFile(
          std::move(new_handle->shm_handle), &platform_file);
      CHECK_EQ(unwrap_result, MOJO_RESULT_OK);
#if defined(OS_WIN)
      base::SharedMemoryHandle shm_handle =
          base::SharedMemoryHandle(platform_file, base::GetCurrentProcId());
#else
      base::SharedMemoryHandle shm_handle =
          base::SharedMemoryHandle(platform_file, true);
#endif
      new_handles.emplace_back(new_handle->id, shm_handle);
    }
    bool result = bulk_buffer_reader_.ImportBackings(std::move(new_handles));
    CHECK(result);
  }
  frame_for_commit_ = std::move(frame);
  if (will_wait_for_activation)
    wait_for_activation_state_ = kWaitForActivationPrepared;

  scheduler_.NotifyBeginMainFrameStarted(
      base::TimeTicks() /* TODO(hackathon): main_thread_start_time */);
  host_impl_.ReadyToCommit();

  scheduler_.NotifyReadyToCommit();
}

void ContentFrameSink::DidActivateSyncTree() {
  TRACE_EVENT0("cc", "ContentFrameSink::DidActivateSyncTree");
  DCHECK(wait_for_activation_state_ == kWaitForActivationNone ||
         wait_for_activation_state_ == kWaitForActivationCommitted);
  if (activation_callback_) {
    DCHECK_NE(wait_for_activation_state_, kWaitForActivationNone);
    wait_for_activation_state_ = kWaitForActivationNone;
    activation_callback_.Run();
    activation_callback_.Reset();
  } else {
    if (wait_for_activation_state_ == kWaitForActivationCommitted)
      wait_for_activation_state_ = kWaitForActivationActivated;
  }
}

void ContentFrameSink::WaitForActivation(
    const WaitForActivationCallback& callback) {
  DCHECK_NE(wait_for_activation_state_, kWaitForActivationNone);
  DCHECK(!activation_callback_);
  if (wait_for_activation_state_ == kWaitForActivationActivated) {
    wait_for_activation_state_ = kWaitForActivationNone;
    callback.Run();
  } else {
    activation_callback_ = callback;
  }
}

AdditionGroup<gfx::ScrollOffset> ScrollOffsetFromMojom(
    const gfx::mojom::ScrollOffsetPtr& mojom) {
  return AdditionGroup<gfx::ScrollOffset>(
      gfx::ScrollOffset(mojom->x, mojom->y));
}

void ContentFrameSink::BeginMainFrameAborted(CommitEarlyOutReason reason) {
  TRACE_EVENT0("cc", "ContentFrameSink::BeginMainFrameAborted");
#if 0
  // TODO(hackathon):
  if (CommitEarlyOutHandledCommit(reason)) {
    SetInputThrottledUntilCommitOnImpl(false);
  }
#endif
  host_impl_.BeginMainFrameAborted(reason);
  scheduler_.NotifyBeginMainFrameStarted(
      base::TimeTicks() /* TODO(hackathon): heh. */);
  scheduler_.BeginMainFrameAborted(reason);
}

void ContentFrameSink::FinishCommit() {
  TRACE_EVENT0("cc", "ContentFrameSink::FinishCommit");
  mojom::ContentFrame* frame = frame_for_commit_.get();
  LayerTreeImpl* sync_tree = host_impl_.sync_tree();

  if (frame->next_commit_forces_redraw)
    sync_tree->ForceRedrawNextActivation();

  sync_tree->set_source_frame_number(frame->source_frame);

  if (frame->needs_full_tree_sync) {
    TRACE_EVENT0("cc",
                 "ContentFrameSink::FinishCommit full tree sync create layers");
    std::unique_ptr<OwnedLayerImplList> old_pending_layers(
        sync_tree->DetachLayers());
    OwnedLayerImplMap old_pending_layer_map;
    for (auto& it : *old_pending_layers)
      old_pending_layer_map[it->id()] = std::move(it);

    auto create_layer = [&old_pending_layer_map,
                         sync_tree](cc::mojom::LayerStructure* mojom) {
      int id = mojom->layer_id;
      std::unique_ptr<LayerImpl> layer = std::move(old_pending_layer_map[id]);
      if (!layer) {
        switch (mojom->layer_type) {
          case cc::mojom::LayerType::BASE:
            layer = LayerImpl::Create(sync_tree, id);
            break;
          case cc::mojom::LayerType::PICTURE:
            layer = PictureLayerImpl::Create(sync_tree, id, mojom->is_mask);
            break;
          case cc::mojom::LayerType::PAINTED_SCROLLBAR:
            layer = PaintedScrollbarLayerImpl::Create(
                sync_tree, id, mojom->scrollbar_orientation_is_horizontal
                                   ? ScrollbarOrientation::HORIZONTAL
                                   : ScrollbarOrientation::VERTICAL);
            break;
          case cc::mojom::LayerType::SOLID_COLOR:
            layer = SolidColorLayerImpl::Create(sync_tree, id);
            break;
          case cc::mojom::LayerType::SURFACE:
            layer = SurfaceLayerImpl::Create(sync_tree, id);
            break;
        }
      }
      return layer;
    };

    sync_tree->ClearLayerList();
    auto& tree = frame->layer_tree;
    if (tree) {
      for (const auto& layer_structure : tree->layers) {
        auto layer = create_layer(layer_structure.get());
        sync_tree->AddToLayerList(layer.get());
        sync_tree->AddLayer(std::move(layer));
      }
    }
    sync_tree->OnCanDrawStateChangedForTree();
  }
  sync_tree->set_needs_full_tree_sync(frame->needs_full_tree_sync);

#if 0
  // TODO(hackathon): HUD
  if (hud_layer_.get()) {
    LayerImpl* hud_impl = sync_tree->LayerById(hud_layer_->id());
    sync_tree->set_hud_layer(static_cast<HeadsUpDisplayLayerImpl*>(hud_impl));
  } else {
    sync_tree->set_hud_layer(NULL);
  }
#endif

  sync_tree->set_background_color(frame->background_color);
  sync_tree->set_has_transparent_background(frame->has_transparent_background);
  sync_tree->set_have_scroll_event_handlers(frame->have_scroll_event_handlers);
#if 0
  // TODO(hackathon): event listener properties
  sync_tree->set_event_listener_properties(
      EventListenerClass::kTouchStartOrMove,
      event_listener_properties(EventListenerClass::kTouchStartOrMove));
  sync_tree->set_event_listener_properties(
      EventListenerClass::kMouseWheel,
      event_listener_properties(EventListenerClass::kMouseWheel));
  sync_tree->set_event_listener_properties(
      EventListenerClass::kTouchEndOrCancel,
      event_listener_properties(EventListenerClass::kTouchEndOrCancel));
#endif

  sync_tree->SetViewportLayersFromIds(
      frame->overscroll_elasticity_layer, frame->page_scale_layer,
      frame->inner_viewport_scroll_layer, frame->outer_viewport_scroll_layer);

#if 0
  // TODO(hackathon): selection
  sync_tree->RegisterSelection(selection_);
#endif

#if 0
  // TODO(hackathon): can keep this state in the Service (it doesn't need to
  // flow back to the LTH)

  bool property_trees_changed_on_active_tree =
      sync_tree->IsActiveTree() && sync_tree->property_trees()->changed;
  // Property trees may store damage status. We preserve the sync tree damage
  // status by pushing the damage status from sync tree property trees to main
  // thread property trees or by moving it onto the layers.
  if (root_layer_ && property_trees_changed_on_active_tree) {
    if (property_trees_.sequence_number ==
        sync_tree->property_trees()->sequence_number)
      sync_tree->property_trees()->PushChangeTrackingTo(&property_trees_);
    else
      sync_tree->MoveChangeTrackingToLayers();
  }
#endif

  // Setting property trees must happen before pushing the page scale.
  PropertyTrees* property_trees = sync_tree->property_trees();
  {
    TRACE_EVENT0("cc", "ContentFrameSink::FinishCommit property trees");
    bool property_trees_sequence_changed = false;
    {
      auto& source = *frame->property_trees;
      auto* dest = property_trees;
      dest->non_root_surfaces_enabled = source.non_root_surfaces_enabled;
      dest->changed = source.changed;
      dest->full_tree_damaged = source.full_tree_damaged;
      property_trees_sequence_changed =
          (dest->sequence_number != source.sequence_number);
      dest->sequence_number = source.sequence_number;
      dest->verify_transform_tree_calculations =
          source.verify_transform_tree_calculations;
    }
    {
      auto& source = *frame->property_trees->transform_tree;
      auto* dest = &property_trees->transform_tree;
      dest->source_to_parent_updates_allowed_ =
          source.source_to_parent_updates_allowed;
      dest->page_scale_factor_ = source.page_scale_factor;
      dest->device_scale_factor_ = source.device_scale_factor;
      dest->device_transform_scale_factor_ =
          source.device_transform_scale_factor;
      dest->nodes_affected_by_inner_viewport_bounds_delta_ =
          source.nodes_affected_by_inner_viewport_bounds_delta;
      dest->nodes_affected_by_outer_viewport_bounds_delta_ =
          source.nodes_affected_by_outer_viewport_bounds_delta;
      size_t vec_bytes = source.nodes.size();
      DCHECK_EQ(vec_bytes % sizeof(TransformNode),
                0u);  // TODO(hackathon): validate
      dest->nodes_.resize(vec_bytes / sizeof(TransformNode));
      memcpy(dest->nodes_.data(), source.nodes.data(), vec_bytes);

      dest->cached_data_.resize(source.cached_data.size());
      for (size_t i = 0; i < source.cached_data.size(); ++i) {
        auto& src_data = *source.cached_data[i];
        auto* dst_data = &dest->cached_data_[i];
        dst_data->from_target = src_data.from_target;
        dst_data->to_target = src_data.to_target;
        dst_data->from_screen = src_data.from_screen;
        dst_data->to_screen = src_data.to_screen;
        dst_data->target_id = src_data.target_id;
        dst_data->content_target_id = src_data.content_target_id;
      }
    }
    {
      auto& source = *frame->property_trees->effect_tree;
      auto* dest = &property_trees->effect_tree;
      dest->mask_replica_layer_ids_ = source.mask_replica_layer_ids;
      size_t vec_bytes = source.nodes.size();
      DCHECK_EQ(vec_bytes % sizeof(EffectNode), 0u);
      dest->nodes_.resize(vec_bytes / sizeof(EffectNode));
      memcpy(dest->nodes_.data(), source.nodes.data(), vec_bytes);
    }
    {
      auto& source = *frame->property_trees->scroll_tree;
      auto* dest = &property_trees->scroll_tree;
      dest->currently_scrolling_node_id_ = source.currently_scrolling_node_id;
      // Scroll offsets are done below.
      size_t vec_bytes = source.nodes.size();
      DCHECK_EQ(vec_bytes % sizeof(ScrollNode), 0u);
      dest->nodes_.resize(vec_bytes / sizeof(ScrollNode));
      memcpy(dest->nodes_.data(), source.nodes.data(), vec_bytes);
    }
    {
      auto& source = *frame->property_trees->clip_tree;
      auto* dest = &property_trees->clip_tree;
      size_t vec_bytes = source.nodes.size();
      DCHECK_EQ(vec_bytes % sizeof(ClipNode), 0u);
      dest->nodes_.resize(vec_bytes / sizeof(ClipNode));
      memcpy(dest->nodes_.data(), source.nodes.data(), vec_bytes);
    }

    // TODO(hackathon) : do this
    // property_trees->effect_tree.PushCopyRequestsTo(&property_trees_.effect_tree);
    property_trees->is_main_thread = false;
    property_trees->is_active = sync_tree->IsActiveTree();
    property_trees->transform_tree.set_source_to_parent_updates_allowed(false);
    // The value of some effect node properties (like is_drawn) depends on
    // whether we are on the active tree or not. So, we need to update the
    // effect tree.
    if (sync_tree->IsActiveTree())
      property_trees->effect_tree.set_needs_update(true);
    if (property_trees_sequence_changed)
      property_trees->ResetCachedData();
  }

  sync_tree->PushPageScaleFromMainThread(frame->page_scale_factor,
                                         frame->min_page_scale_factor,
                                         frame->max_page_scale_factor);
  sync_tree->elastic_overscroll()->PushFromMainThread(
      frame->elastic_overscroll);
  if (sync_tree->IsActiveTree())
    sync_tree->elastic_overscroll()->PushPendingToActive();

#if 0
  // TODO(hackathon): swap promises
  sync_tree->PassSwapPromises(&swap_promise_list_);
#endif

#if 0
  // TODO(hackathon): top controls
  sync_tree->set_top_controls_shrink_blink_size(
      top_controls_shrink_blink_size_);
  sync_tree->set_top_controls_height(top_controls_height_);
  sync_tree->PushTopControlsFromMainThread(top_controls_shown_ratio_);
#endif

  host_impl_.SetHasGpuRasterizationTrigger(
      frame->has_gpu_rasterization_trigger);
  host_impl_.SetContentIsSuitableForGpuRasterization(
      frame->content_is_suitable_for_gpu_rasterization);

  host_impl_.SetViewportSize(frame->device_viewport_size);
  // TODO(senorblanco): Move this up so that it happens before GPU rasterization
  // properties are set, since those trigger an update of GPU rasterization
  // status, which depends on the device scale factor. (crbug.com/535700)
  sync_tree->SetDeviceScaleFactor(frame->device_scale_factor);
  sync_tree->set_painted_device_scale_factor(
      frame->painted_device_scale_factor);
#if 0
  // TODO(hackathon): debug state
  host_impl->SetDebugState(debug_state_);
#endif

#if 0
  // TODO(hackathon): pending page scale animation
  if (pending_page_scale_animation_) {
    sync_tree->SetPendingPageScaleAnimation(
        std::move(pending_page_scale_animation_));
  }
#endif

  // TODO(hackathon): UI resources
  UIResourceRequestQueue ui_resource_request_queue;
  auto& ui_resource_requests = frame->ui_resource_request_queue;
  auto create_ui_resource_request =
      [](cc::mojom::UIResourceRequestProperties* mojom) {
        int uid = mojom->uid;
        std::unique_ptr<UIResourceRequest> request;
        std::unique_ptr<UIResourceBitmap> bitmap;
        switch (mojom->type) {
          case cc::mojom::UIResourceRequestType::CREATE_REQUEST:
            DCHECK(mojom->bitmap);
            bitmap = cc::UIResourceBitmap::CreateFromMojom(mojom->bitmap.get());
            request = base::WrapUnique(new UIResourceRequest(
                UIResourceRequest::UI_RESOURCE_CREATE, uid, *bitmap.get()));
            break;
          case cc::mojom::UIResourceRequestType::DELETE:
            request = base::WrapUnique(new UIResourceRequest(
                UIResourceRequest::UI_RESOURCE_DELETE, uid));
            break;
          case cc::mojom::UIResourceRequestType::INVALID:
            // NOTREACHED.
            break;
        }
        return request;
      };

  if (ui_resource_requests) {
    for (const auto& request : ui_resource_requests->ui_resource_requests) {
      auto ui_resource_request = create_ui_resource_request(request.get());
      if (ui_resource_request)
        ui_resource_request_queue.push_back(*ui_resource_request.get());
    }
  }
  if (!ui_resource_request_queue.empty()) {
    sync_tree->set_ui_resource_request_queue(ui_resource_request_queue);
  }

  DCHECK(!sync_tree->ViewportSizeInvalid());

  sync_tree->set_has_ever_been_drawn(false);

  {
    TRACE_EVENT0("cc", "ContentFrameSink::PushProperties");

    ContentFrameReaderContext context = {frame, bulk_buffer_reader_};

    for (const auto& layer_properties : frame->layer_properties) {
      ProxyImageGenerator::ScopedBindFactory scoped_bind(
          host_impl_.image_decode_proxy());
      LayerImpl* layer = sync_tree->LayerById(layer_properties->layer_id);
      DCHECK(layer);
      layer->ReadPropertiesMojom(context, layer_properties.get());
    }

    // This must happen after synchronizing property trees and after push
    // properties, which updates property tree indices, but before animation
    // host pushes properties as animation host push properties can change
    // Animation::InEffect and we want the old InEffect value for updating
    // property tree scrolling and animation.
    sync_tree->UpdatePropertyTreeScrollingAndAnimationFromMainThread();

    TRACE_EVENT0("cc", "ContentFrameSink::AnimationHost::PushProperties");
#if 0
    // TODO(hackathon): animations
    DCHECK(host_impl->animation_host());
    layer_tree_.animation_host()->PushPropertiesTo(host_impl->animation_host());
#endif
  }

  // This must happen after synchronizing property trees and after pushing
  // properties, which updates the clobber_active_value flag.

  // TODO(hackathon): pending/active business seems really wrong...
  ScrollTree::ScrollOffsetMap new_scroll_offset_map;
  for (const auto& pair :
       frame->property_trees->scroll_tree->layer_id_to_scroll_offset_map) {
    scoped_refptr<SyncedScrollOffset> synced = new SyncedScrollOffset;
    synced->pending_base_ = ScrollOffsetFromMojom(pair.second->pending_base);
    synced->active_base_ = ScrollOffsetFromMojom(pair.second->active_base);
    synced->active_delta_ = ScrollOffsetFromMojom(pair.second->active_delta);
    synced->reflected_delta_in_main_tree_ =
        ScrollOffsetFromMojom(pair.second->reflected_delta_in_main_tree);
    synced->reflected_delta_in_pending_tree_ =
        ScrollOffsetFromMojom(pair.second->reflected_delta_in_pending_tree);
    synced->clobber_active_value_ = pair.second->clobber_active_value;
    new_scroll_offset_map[pair.first] = synced;
  }
  property_trees->scroll_tree.UpdateScrollOffsetMap(&new_scroll_offset_map,
                                                    sync_tree);

  // Done deserializing, can return backings to client.
  if (!frame->backings.empty())
    content_frame_sink_client_->OnBackingsReturned(std::move(frame->backings));

  // TODO(weiliangc): This crashes once surface ids start changing.
  // The surface ID changed during BeginCommitSync.
  if (0) {
    TRACE_EVENT0("cc",
                 "ContentFrameSink::FinishCommit content frame aggregation");
    // Hackathon technically only combines one content frame because of reasons.
    cc::ContentFrame new_content_frame;
    new_content_frame.Set(*sync_tree);
    if (display_) {
      display_->CommitContentFrame(std::move(new_content_frame));
      AggregatedContentFrame result_frame = display_->AggregateContentFrames();
    }
  }

#if 0
  // TODO(hackathon): micro benchmarks
  micro_benchmark_controller_.ScheduleImplBenchmarks(host_impl);
#endif
}

void ContentFrameSink::OnClientConnectionLost() {
  client_connection_lost_ = true;
  display_compositor_->OnContentFrameSinkClientConnectionLost(
      compositor_frame_sink_id_, private_connection_lost_);
}

void ContentFrameSink::OnPrivateConnectionLost() {
  private_connection_lost_ = true;
  display_compositor_->OnContentFrameSinkPrivateConnectionLost(
      compositor_frame_sink_id_, client_connection_lost_);
}

void ContentFrameSink::ScheduledActionCommit() {
  TRACE_EVENT0("cc", "ContentFrameSink::ScheduledActionCommit");
  DCHECK(wait_for_activation_state_ == kWaitForActivationNone ||
         wait_for_activation_state_ == kWaitForActivationPrepared);
  host_impl_.BeginCommit();

  FinishCommit();
  frame_for_commit_ = nullptr;

  if (wait_for_activation_state_ == kWaitForActivationPrepared)
    wait_for_activation_state_ = kWaitForActivationCommitted;

  scheduler_.DidCommit();
  host_impl_.CommitComplete();
}

void ContentFrameSink::DeleteBackings(const std::vector<uint32_t>& backings) {
  bulk_buffer_reader_.DeleteBackings(backings);
}

void ContentFrameSink::Destroy(const DestroyCallback& callback) {
  TRACE_EVENT0("cc", "ContentFrameSink::Destroy");
  scheduler_.SetVisible(false);
  scheduler_.DidLoseOutputSurface();
  host_impl_.ReleaseOutputSurface();
  output_surface_ = nullptr;
  display_ = nullptr;
  callback.Run();
}

}  // namespace cc
