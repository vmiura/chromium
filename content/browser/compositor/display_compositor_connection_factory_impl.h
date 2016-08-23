// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_COMPOSITOR_DISPLAY_COMPOSITOR_CONNECTION_FACTORY_IMPL_H_
#define CONTENT_BROWSER_COMPOSITOR_DISPLAY_COMPOSITOR_CONNECTION_FACTORY_IMPL_H_

#include "cc/host/display_compositor_connection.h"
#include "cc/host/display_compositor_host.h"
#include "cc/ipc/compositor.mojom.h"

namespace content {

// DisplayCompositorConnectionFactoryImpl is a content-specific implementation
// of DisplayCompositorConnectionFactory to talk to, and if necessary, reboot
// the GPU process created by content/ if it crashes to create a connection
// to the display compositor. An alternative implementation of this interface
// could provide a connection to the display compositor living in the browser
// or in the Mus+Ash window server.
// This particular implementation is created on the browser process' UI thread,
// but accessed on the browser process' IO thread.
// DisplayCompositorConnectionFactoryImpl is a
// DisplayCompositorConnectionObserver but also takes in a
// DisplayCompositorConnectionObserver. The difference being calls to its
// internal methods happen on the IO thread but are relayed to the UI thread to
// the external observer. It is assumed that the external observer outlives this
// object.
class DisplayCompositorConnectionFactoryImpl
    : public cc::DisplayCompositorConnectionFactory,
      public cc::DisplayCompositorConnectionObserver {
 public:
  DisplayCompositorConnectionFactoryImpl();

  // These are main (UI) thread observers that observe updates coming
  // from the DisplayCompositorConnection's thread (typically IO thread).
  void AddObserver(DisplayCompositorConnectionObserver* observer);
  void RemoveObserver(DisplayCompositorConnectionObserver* observer);

  // DisplayCompositorConnectionFactory implementation:
  cc::DisplayCompositorConnection* GetDisplayCompositorConnection() override;

 private:
  ~DisplayCompositorConnectionFactoryImpl() override;

  // DisplayCompositorConnectionObserver implementation:
  void OnSurfaceCreated(const gfx::Size& frame_size,
                        const cc::SurfaceId& surface_id) override;

  friend class base::RefCountedThreadSafe<
      DisplayCompositorConnectionFactoryImpl>;

  base::ObserverList<DisplayCompositorConnectionObserver> observers_;
  scoped_refptr<base::SingleThreadTaskRunner> main_task_runner_;
  std::unique_ptr<cc::DisplayCompositorConnection> display_compositor_;

  DISALLOW_COPY_AND_ASSIGN(DisplayCompositorConnectionFactoryImpl);
};

}  // namespace content

#endif  // CONTENT_BROWSER_COMPOSITOR_DISPLAY_COMPOSITOR_CONNECTION_FACTORY_IMPL_H_
