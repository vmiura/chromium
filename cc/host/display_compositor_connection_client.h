// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_HOST_DISPLAY_COMPOSITOR_CONNECTION_CLIENT_H_
#define CC_HOST_DISPLAY_COMPOSITOR_CONNECTION_CLIENT_H_

namespace cc {

class DisplayCompositorConnectionClient {
 public:
  virtual ~DisplayCompositorConnectionClient() {}
  virtual void OnConnectionLost() {}
};

}  // namespace cc

#endif  // CC_HOST_DISPLAY_COMPOSITOR_CONNECTION_CLIENT_H_
