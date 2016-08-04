// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_CLIENT_COMPOSITOR_PROXY_H_
#define CC_CLIENT_COMPOSITOR_PROXY_H_

#include "cc/client/client_export.h"
#include "cc/ipc/compositor.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace cc {

class CC_CLIENT_EXPORT CompositorProxy : public cc::mojom::CompositorClient {
 public:
  CompositorProxy(cc::mojom::CompositorPtr compositor,
                  cc::mojom::CompositorClientRequest compositor_client_request);
  ~CompositorProxy() override;

  void set_delegate(mojom::CompositorClient* delegate) { delegate_ = delegate; }

  // cc::mojom::CompositorClient implementation.
  void OnCompositorCreated(uint32_t client_id) override;
  void OnBeginMainFrame(uint32_t begin_frame_id, const BeginFrameArgs& args) override;
  void OnBeginMainFrameNotExpectedSoon() override;
  void OnDidCompletePageScaleAnimation() override;
  void OnDidCommitAndDrawFrame() override;
  void OnDidCompleteSwapBuffers() override;
  void OnRendererCapabilities(
      const cc::RendererCapabilities& capabilities) override;

  void RegisterChildCompositor(uint32_t client_id) {
    compositor_->RegisterChildCompositor(client_id);
  }
  void SetNeedsBeginMainFrame() { compositor_->SetNeedsBeginMainFrame(); }
  void SetNeedsRedraw(const gfx::Rect& damage_rect) { compositor_->SetNeedsRedraw(damage_rect); }
  void SetVisible(bool visible) { compositor_->SetVisible(visible); }
  void Commit(bool hold_commit_for_activation, mojom::ContentFramePtr frame) {
    compositor_->Commit(hold_commit_for_activation, std::move(frame));
  }
  void Destroy() { compositor_->Destroy(); }

 private:
  cc::mojom::CompositorPtr compositor_;
  mojo::Binding<cc::mojom::CompositorClient> binding_;
  mojom::CompositorClient* delegate_;
};

}  // namespace cc

#endif  // CC_CLIENT_COMPOSITOR_PROXY_H_
