// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_HOST_DISPLAY_COMPOSITOR_CONNECTION_FACTORY_H_
#define CC_HOST_DISPLAY_COMPOSITOR_CONNECTION_FACTORY_H_

#include "base/memory/ref_counted.h"

namespace cc {

class DisplayCompositorConnection;

class DisplayCompositorConnectionFactory
    : public base::RefCountedThreadSafe<DisplayCompositorConnectionFactory> {
 public:
  // Perhaps this should be a const ref instead?
  virtual DisplayCompositorConnection* GetDisplayCompositorConnection() = 0;

 protected:
  virtual ~DisplayCompositorConnectionFactory() {}

 private:
  friend class base::RefCountedThreadSafe<DisplayCompositorConnectionFactory>;
};

}  // namespace cc

#endif  // CC_HOST_DISPLAY_COMPOSITOR_CONNECTION_FACTORY_H_
