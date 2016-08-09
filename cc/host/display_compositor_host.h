// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_HOST_DISPLAY_COMPOSITOR_HOST_H_
#define CC_HOST_DISPLAY_COMPOSITOR_HOST_H_

#include "cc/ipc/compositor.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace cc {

struct DisplayCompositorConnection {
  DisplayCompositorConnection();
  DisplayCompositorConnection(DisplayCompositorConnection&& other);
  ~DisplayCompositorConnection();
  mojom::DisplayCompositorPtr compositor;
  mojom::DisplayCompositorClientRequest client_request;
};

class DisplayCompositorHost : public mojom::DisplayCompositorHost,
                              public mojom::DisplayCompositorClient {
 public:
  class Delegate : public base::RefCounted<Delegate> {
   public:
    virtual DisplayCompositorConnection GetDisplayCompositorConnection() = 0;

   protected:
    virtual ~Delegate() {}

   private:
    friend class base::RefCounted<Delegate>;
  };
  static void Create(int32_t process_id,
                     scoped_refptr<Delegate> delegate,
                     mojom::DisplayCompositorHostRequest request);

  ~DisplayCompositorHost() override;

  int32_t process_id() const { return process_id_; }

  // DisplayCompositorHost implementation.
  void CreateCompositor(int32_t routing_id,
                        mojom::LayerTreeSettingsPtr settings,
                        mojom::CompositorRequest compositor,
                        mojom::CompositorClientPtr client) override;

 private:
  DisplayCompositorHost(int32_t process_id,
                        scoped_refptr<Delegate> delegate,
                        mojom::DisplayCompositorHostRequest request);

  const int32_t process_id_;
  scoped_refptr<Delegate> delegate_;
  uint32_t next_compositor_id_ = 1;
  mojom::DisplayCompositorPtr display_compositor_;
  mojo::Binding<mojom::DisplayCompositorClient> client_binding_;
  mojo::Binding<mojom::DisplayCompositorHost> binding_;

  DISALLOW_COPY_AND_ASSIGN(DisplayCompositorHost);
};

}  // namespace cc

#endif  // CC_HOST_DISPLAY_COMPOSITOR_HOST_H_
