// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_CLIENT_COMPOSITOR_PROXY_H_
#define CC_CLIENT_COMPOSITOR_PROXY_H_

#include "cc/client/client_export.h"
#include "cc/ipc/compositor.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace cc {

class CC_CLIENT_EXPORT CompositorProxyDelegate {
 public:
  ~CompositorProxyDelegate() {}
  virtual void OnCompositorCreated() = 0;
  virtual void OnBeginMainFrame(uint32_t begin_frame_id, const BeginFrameArgs& begin_frame_args) = 0;
};

class CC_CLIENT_EXPORT CompositorProxy : public cc::mojom::CompositorClient {
 public:
  CompositorProxy(cc::mojom::CompositorPtr compositor,
                  cc::mojom::CompositorClientRequest compositor_client_request);
  ~CompositorProxy() override;

  void set_delegate(CompositorProxyDelegate* delegate) { delegate_ = delegate; }

  // cc::mojom::CompositorClient implementation.
  void OnCompositorCreated() override;
  void OnBeginMainFrame(uint32_t begin_frame_id, const BeginFrameArgs& args) override;

  void SetNeedsBeginMainFrame() { compositor_->SetNeedsBeginMainFrame(); }
  void Commit(bool hold_commit_for_activation, mojom::ContentFramePtr frame) { compositor_->Commit(hold_commit_for_activation, std::move(frame)); }

 private:
  cc::mojom::CompositorPtr compositor_;
  mojo::Binding<cc::mojom::CompositorClient> binding_;
  CompositorProxyDelegate* delegate_;
};

}  // namespace cc

#endif  // CC_CLIENT_COMPOSITOR_PROXY_H_
