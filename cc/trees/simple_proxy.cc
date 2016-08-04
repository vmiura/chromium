// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/trees/simple_proxy.h"

#include <algorithm>
#include <string>

#include "base/trace_event/trace_event.h"
#include "base/trace_event/trace_event_argument.h"
#include "base/trace_event/trace_event_synthetic_delay.h"
#include "cc/animation/animation_events.h"
#include "cc/debug/benchmark_instrumentation.h"
#include "cc/debug/devtools_instrumentation.h"
#include "cc/ipc/content_frame.mojom.h"
#include "cc/output/output_surface.h"
#include "cc/output/swap_promise.h"
#include "cc/trees/blocking_task_runner.h"
#include "cc/trees/layer_tree_host.h"
#include "cc/trees/remote_channel_main.h"
#include "cc/trees/scoped_abort_remaining_swap_promises.h"
#include "cc/trees/threaded_channel.h"
#include "mojo/public/cpp/bindings/sync_call_restrictions.h"

namespace cc {

std::unique_ptr<SimpleProxy> SimpleProxy::Create(
    LayerTreeHost* layer_tree_host,
    TaskRunnerProvider* task_runner_provider) {
  std::unique_ptr<SimpleProxy> proxy_main(
      new SimpleProxy(layer_tree_host, task_runner_provider));
  return proxy_main;
}

SimpleProxy::SimpleProxy(LayerTreeHost* layer_tree_host,
                         TaskRunnerProvider* task_runner_provider)
    : layer_tree_host_(layer_tree_host),
      task_runner_provider_(task_runner_provider),
      layer_tree_host_id_(layer_tree_host->id()),
      commit_waits_for_activation_(false),
      started_(false),
      defer_commits_(false),
      begin_frame_requested_(false) {
  TRACE_EVENT0("cc", "SimpleProxy::SimpleProxy");
  DCHECK(task_runner_provider_);
  DCHECK(IsMainThread());
}

SimpleProxy::~SimpleProxy() {
  TRACE_EVENT0("cc", "SimpleProxy::~SimpleProxy");
  DCHECK(IsMainThread());
  DCHECK(!started_);
}

#if 0
void SimpleProxy::SetAnimationEvents(std::unique_ptr<AnimationEvents> events) {
  TRACE_EVENT0("cc", "SimpleProxy::SetAnimationEvents");
  DCHECK(IsMainThread());
  layer_tree_host_->SetAnimationEvents(std::move(events));
}
#endif

void SimpleProxy::InitializeCompositor(
    std::unique_ptr<CompositorProxy> compositor) {
  compositor_ = std::move(compositor);
  compositor_->set_delegate(this);
  if (needs_begin_frame_when_ready_) {
    needs_begin_frame_when_ready_ = false;
    SetNeedsBeginFrame();
  }
  if (needs_redraw_when_ready_) {
    needs_redraw_when_ready_ = false;
    SetNeedsRedraw(needs_redraw_rect_when_ready_);
  }
  if (needs_set_visible_when_ready_) {
    needs_set_visible_when_ready_ = false;
    SetVisible(needs_set_visible_value_when_ready_);
  }
}

void SimpleProxy::FinishAllRendering() {
  DCHECK(IsMainThread());
  DCHECK(!defer_commits_);
}

bool SimpleProxy::IsStarted() const {
  DCHECK(IsMainThread());
  return started_;
}

bool SimpleProxy::CommitToActiveTree() const {
  // TODO(piman): hackathon:??
  return false;
}

void SimpleProxy::SetOutputSurface(OutputSurface* output_surface) {
  NOTREACHED();
}

void SimpleProxy::SetSurfaceClientId(uint32_t client_id) {
  surface_id_ = SurfaceId(client_id, 1, 1);
}

void SimpleProxy::SetVisible(bool visible) {
  TRACE_EVENT1("cc", "SimpleProxy::SetVisible", "visible", visible);
  if (!compositor_) {
    needs_set_visible_when_ready_ = true;
    needs_set_visible_value_when_ready_ = visible;
    return;
  }
  compositor_->SetVisible(visible);
}

const RendererCapabilities& SimpleProxy::GetRendererCapabilities() const {
  // TODO(piman): hackathon ??
  DCHECK(IsMainThread());
  return renderer_capabilities_;
}

void SimpleProxy::SetNeedsAnimate() {
  DCHECK(IsMainThread());
  // TODO(piman): hackathon ??
  SetNeedsBeginFrame();
}

void SimpleProxy::SetNeedsUpdateLayers() {
  DCHECK(IsMainThread());
  // TODO(piman): hackathon ??
  SetNeedsBeginFrame();
}

void SimpleProxy::SetNeedsCommit() {
  DCHECK(IsMainThread());
  // TODO(piman): hackathon ??
  SetNeedsBeginFrame();
}

void SimpleProxy::SetNeedsRedraw(const gfx::Rect& damage_rect) {
  TRACE_EVENT0("cc", "SimpleProxy::SetNeedsRedraw");
  DCHECK(IsMainThread());
  // TODO(piman): hackathon ??
  if (!compositor_) {
    needs_redraw_when_ready_ = true;
    needs_redraw_rect_when_ready_ = damage_rect;
    return;
  }
  compositor_->SetNeedsRedraw(damage_rect);
}

void SimpleProxy::SetNextCommitWaitsForActivation() {
  DCHECK(IsMainThread());
  commit_waits_for_activation_ = true;
}

void SimpleProxy::NotifyInputThrottledUntilCommit() {
  DCHECK(IsMainThread());
  // TODO(piman): hackathon ??
  NOTIMPLEMENTED();
}

void SimpleProxy::SetDeferCommits(bool defer_commits) {
  DCHECK(IsMainThread());
  if (defer_commits_ == defer_commits)
    return;

  defer_commits_ = defer_commits;
  if (defer_commits_)
    TRACE_EVENT_ASYNC_BEGIN0("cc", "SimpleProxy::SetDeferCommits", this);
  else
    TRACE_EVENT_ASYNC_END0("cc", "SimpleProxy::SetDeferCommits", this);

  if (!defer_commits_)
    SetNeedsBeginFrame();
}

bool SimpleProxy::CommitRequested() const {
  DCHECK(IsMainThread());
  return begin_frame_requested_;
}

bool SimpleProxy::BeginMainFrameRequested() const {
  DCHECK(IsMainThread());
  return begin_frame_requested_;
}

void SimpleProxy::MainThreadHasStoppedFlinging() {
  DCHECK(IsMainThread());
  // TODO(piman): hackathon ??
  NOTIMPLEMENTED();
}

void SimpleProxy::Start(
    std::unique_ptr<BeginFrameSource> external_begin_frame_source) {
  DCHECK(IsMainThread());
  DCHECK(!layer_tree_host_->settings().use_external_begin_frame_source ||
         external_begin_frame_source);

  started_ = true;
}

void SimpleProxy::Stop() {
  TRACE_EVENT0("cc", "SimpleProxy::Stop");
  DCHECK(IsMainThread());
  DCHECK(started_);

  if (compositor_) {
    compositor_->Destroy();
    compositor_->set_delegate(nullptr);
  }

  layer_tree_host_ = nullptr;
  started_ = false;
}

void SimpleProxy::SetMutator(std::unique_ptr<LayerTreeMutator> mutator) {
  TRACE_EVENT0("compositor-worker", "ThreadProxy::SetMutator");
  // TODO(piman): hackathon ??
  NOTIMPLEMENTED();
}

bool SimpleProxy::SupportsImplScrolling() const {
  return true;
}

bool SimpleProxy::MainFrameWillHappenForTesting() {
  DCHECK(IsMainThread());
  return begin_frame_requested_;
}

void SimpleProxy::ReleaseOutputSurface() {
  DCHECK(IsMainThread());
  DCHECK(layer_tree_host_->output_surface_lost());
  // TODO(piman): hackathon ??
  NOTREACHED();
}

void SimpleProxy::UpdateTopControlsState(TopControlsState constraints,
                                       TopControlsState current,
                                       bool animate) {
  DCHECK(IsMainThread());
  // TODO(piman): hackathon ??
  NOTIMPLEMENTED();
}

void SimpleProxy::SetNeedsBeginFrame() {
  DCHECK(IsMainThread());
  if (!compositor_) {
    needs_begin_frame_when_ready_ = true;
    return;
  }

  if (defer_commits_)
    return;
  if (begin_frame_requested_)
    return;
  begin_frame_requested_ = true;
  compositor_->SetNeedsBeginMainFrame();
}

bool SimpleProxy::IsMainThread() const {
  return task_runner_provider_->IsMainThread();
}

void SimpleProxy::OnCompositorCreated() {}

void SimpleProxy::OnBeginMainFrame(
    uint32_t begin_frame_id, const BeginFrameArgs& begin_frame_args) {
  benchmark_instrumentation::ScopedBeginFrameTask begin_frame_task(
      benchmark_instrumentation::kDoBeginFrame,
      begin_frame_id);

  TRACE_EVENT_SYNTHETIC_DELAY_BEGIN("cc.BeginMainFrame");
  DCHECK(IsMainThread());
  DCHECK(begin_frame_requested_);
  begin_frame_requested_ = false;

  if (defer_commits_) {
    TRACE_EVENT_INSTANT0("cc", "EarlyOut_DeferCommit",
                         TRACE_EVENT_SCOPE_THREAD);
    return;
  }

  // If the commit finishes, LayerTreeHost will transfer its swap promises to
  // LayerTreeImpl. The destructor of ScopedSwapPromiseChecker aborts the
  // remaining swap promises.
  ScopedAbortRemainingSwapPromises swap_promise_checker(layer_tree_host_);

  if (!layer_tree_host_->visible()) {
    TRACE_EVENT_INSTANT0("cc", "EarlyOut_NotVisible", TRACE_EVENT_SCOPE_THREAD);
    // TODO(piman): hackathon ??
    NOTIMPLEMENTED();
    return;
  }

  //if (layer_tree_host_->output_surface_lost()) {
  //  TRACE_EVENT_INSTANT0("cc", "EarlyOut_OutputSurfaceLost",
  //                       TRACE_EVENT_SCOPE_THREAD);
  //  // TODO(piman): hackathon ??
  //  NOTREACHED();
  //  return;
  //}

  // TODO(piman): hackathon
  // layer_tree_host_->ApplyScrollAndScale(
  //     begin_main_frame_state->scroll_info.get());

  // if (begin_main_frame_state->begin_frame_callbacks) {
  //   for (auto& callback : *begin_main_frame_state->begin_frame_callbacks)
  //     callback.Run();
  // }

  layer_tree_host_->WillBeginMainFrame();

  layer_tree_host_->BeginMainFrame(begin_frame_args);
  layer_tree_host_->AnimateLayers(begin_frame_args.frame_time);

  // Recreate all UI resources if there were evicted UI resources when the impl
  // thread initiated the commit.
  // if (begin_main_frame_state->evicted_ui_resources)
  //   layer_tree_host_->RecreateUIResources();

  layer_tree_host_->RequestMainFrameUpdate();
  TRACE_EVENT_SYNTHETIC_DELAY_END("cc.BeginMainFrame");

  bool can_cancel_this_commit = false;  // TODO(piman): hackathon ??

  bool should_update_layers = true;  // TODO(piman): hackathon ??
  bool updated = should_update_layers && layer_tree_host_->UpdateLayers();

  layer_tree_host_->WillCommit();
  devtools_instrumentation::ScopedCommitTrace commit_task(
      layer_tree_host_->id());

  if (!updated && can_cancel_this_commit) {
    TRACE_EVENT_INSTANT0("cc", "EarlyOut_NoUpdates", TRACE_EVENT_SCOPE_THREAD);
    // TODO(piman): hackathon ??
    NOTIMPLEMENTED();
    layer_tree_host_->CommitComplete();
    layer_tree_host_->DidBeginMainFrame();
    layer_tree_host_->BreakSwapPromises(SwapPromise::COMMIT_NO_UPDATE);
    return;
  }

  // Notify the impl thread that the main thread is ready to commit. This will
  // begin the commit process, which is blocking from the main thread's
  // point of view, but asynchronously performed on the impl thread,
  // coordinated by the Scheduler.
  {
    TRACE_EVENT0("cc", "SimpleProxy::BeginMainFrame::commit");

    DebugScopedSetMainThreadBlocked main_thread_blocked(task_runner_provider_);

    // This CapturePostTasks should be destroyed before CommitComplete() is
    // called since that goes out to the embedder, and we want the embedder
    // to receive its callbacks before that.
    BlockingTaskRunner::CapturePostTasks blocked(
        task_runner_provider_->blocking_main_thread_task_runner());

    bool hold_commit_for_activation = commit_waits_for_activation_;
    commit_waits_for_activation_ = false;

    mojom::ContentFramePtr frame = mojom::ContentFrame::New();
    layer_tree_host_->GetContentFrame(frame.get());
    mojo::SyncCallRestrictions::ScopedAllowSyncCall sync_call;
    compositor_->Commit(surface_id_, hold_commit_for_activation,
                        std::move(frame));
  }

  layer_tree_host_->CommitComplete();
  layer_tree_host_->DidBeginMainFrame();
}

void SimpleProxy::OnBeginMainFrameNotExpectedSoon() {
  TRACE_EVENT0("cc", "SimpleProxy::BeginMainFrameNotExpectedSoon");
  DCHECK(IsMainThread());
  layer_tree_host_->BeginMainFrameNotExpectedSoon();
}

void SimpleProxy::OnDidCompletePageScaleAnimation() {
  DCHECK(IsMainThread());
  layer_tree_host_->DidCompletePageScaleAnimation();
}

void SimpleProxy::OnDidCommitAndDrawFrame() {
  DCHECK(IsMainThread());
  layer_tree_host_->DidCommitAndDrawFrame();
}

void SimpleProxy::OnDidCompleteSwapBuffers() {
  DCHECK(IsMainThread());
  layer_tree_host_->DidCompleteSwapBuffers();
}

void SimpleProxy::OnRendererCapabilities(
    const cc::RendererCapabilities& capabilities) {
  renderer_capabilities_ = capabilities;
}

}  // namespace cc

