// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_TREES_SIMPLE_PROXY_H_
#define CC_TREES_SIMPLE_PROXY_H_

#include "base/macros.h"
#include "cc/base/cc_export.h"
#include "cc/client/compositor_channel_host.h"
#include "cc/client/compositor_proxy.h"
#include "cc/input/top_controls_state.h"
#include "cc/ipc/compositor.mojom.h"
#include "cc/output/output_surface.h"
#include "cc/output/renderer_capabilities.h"
#include "cc/trees/channel_main.h"
#include "cc/trees/proxy.h"
#include "cc/trees/proxy_common.h"
#include "cc/trees/remote_proto_channel.h"

namespace cc {

class AnimationEvents;
class BeginFrameSource;
class ChannelMain;
class LayerTreeHost;
class LayerTreeMutator;

// This class aggregates all interactions that the impl side of the compositor
// needs to have with the main side.
// The class is created and lives on the main thread.
class CC_EXPORT SimpleProxy : public Proxy, public mojom::CompositorClient {
 public:
  static std::unique_ptr<SimpleProxy> Create(
      LayerTreeHost* layer_tree_host,
      TaskRunnerProvider* task_runner_provider);

  ~SimpleProxy() override;

  // Proxy implementation.
  void InitializeCompositor(
      std::unique_ptr<CompositorProxy> compositor) override;
  void FinishAllRendering() override;
  bool IsStarted() const override;
  bool CommitToActiveTree() const override;
  void SetOutputSurface(OutputSurface* output_surface) override;
  void SetSurfaceClientId(uint32_t client_id) override;
  void SetVisible(bool visible) override;
  const RendererCapabilities& GetRendererCapabilities() const override;
  void SetNeedsAnimate() override;
  void SetNeedsUpdateLayers() override;
  void SetNeedsCommit() override;
  void SetNeedsRedraw(const gfx::Rect& damage_rect) override;
  void SetNextCommitWaitsForActivation() override;
  void NotifyInputThrottledUntilCommit() override;
  void SetDeferCommits(bool defer_commits) override;
  bool CommitRequested() const override;
  bool BeginMainFrameRequested() const override;
  void MainThreadHasStoppedFlinging() override;
  void Start(
      std::unique_ptr<BeginFrameSource> external_begin_frame_source) override;
  void Stop() override;
  bool SupportsImplScrolling() const override;
  void SetMutator(std::unique_ptr<LayerTreeMutator> mutator) override;
  bool MainFrameWillHappenForTesting() override;
  void ReleaseOutputSurface() override;
  void UpdateTopControlsState(TopControlsState constraints,
                              TopControlsState current,
                              bool animate) override;

 protected:
  SimpleProxy(LayerTreeHost* layer_tree_host,
              TaskRunnerProvider* task_runner_provider);

  // mojom::CompositorClient implementation
  void OnCompositorCreated() override;
  void OnBeginMainFrame(uint32_t begin_frame_id, const BeginFrameArgs& begin_frame_args) override;
  void OnBeginMainFrameNotExpectedSoon() override;
  void OnDidCompletePageScaleAnimation() override;
  void OnDidCommitAndDrawFrame() override;
  void OnDidCompleteSwapBuffers() override;
  void OnRendererCapabilities(
      const cc::RendererCapabilities& capabilities) override;

  bool IsMainThread() const;

  void SetNeedsBeginFrame();

  bool needs_begin_frame_when_ready_= false;
  bool needs_redraw_when_ready_ = false;
  gfx::Rect needs_redraw_rect_when_ready_;
  LayerTreeHost* layer_tree_host_;
  TaskRunnerProvider* task_runner_provider_;
  std::unique_ptr<CompositorProxy> compositor_;

  const int layer_tree_host_id_;

  bool commit_waits_for_activation_;

  cc::SurfaceId surface_id_;

  // Set when the Proxy is started using Proxy::Start() and reset when it is
  // stopped using Proxy::Stop().
  bool started_;

  bool defer_commits_;
  bool begin_frame_requested_;

  RendererCapabilities renderer_capabilities_;

  DISALLOW_COPY_AND_ASSIGN(SimpleProxy);
};

}  // namespace cc

#endif  // CC_TREES_SIMPLE_PROXY_H_

