// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/client/compositor_proxy.h"

namespace cc {

CompositorProxy::CompositorProxy(
    cc::mojom::CompositorPtr compositor,
    cc::mojom::CompositorClientRequest compositor_client_request)
    : compositor_(std::move(compositor)),
      binding_(this, std::move(compositor_client_request)),
      delegate_(nullptr) {}

CompositorProxy::~CompositorProxy() = default;

void CompositorProxy::OnCompositorCreated() {
  if (delegate_)
    delegate_->OnCompositorCreated();
}

void CompositorProxy::OnBeginMainFrame(uint32_t begin_frame_id, const BeginFrameArgs& args) {
  if (delegate_)
    delegate_->OnBeginMainFrame(begin_frame_id, args);
}

void CompositorProxy::OnBeginMainFrameNotExpectedSoon() {
  if (delegate_)
    delegate_->OnBeginMainFrameNotExpectedSoon();
}

void CompositorProxy::OnDidCompletePageScaleAnimation() {
  if (delegate_)
    delegate_->OnDidCompletePageScaleAnimation();
}

void CompositorProxy::OnDidCommitAndDrawFrame() {
  if (delegate_)
    delegate_->OnDidCommitAndDrawFrame();
}

void CompositorProxy::OnDidCompleteSwapBuffers() {
  if (delegate_)
    delegate_->OnDidCompleteSwapBuffers();
}

void CompositorProxy::OnRendererCapabilities(
    const cc::RendererCapabilities& capabilities) {
  if (delegate_)
    delegate_->OnRendererCapabilities(capabilities);
}

}  // namespace cc
