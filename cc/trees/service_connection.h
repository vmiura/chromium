// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_TREES_SERVICE_CONNECTION_H_
#define CC_TREES_SERVICE_CONNECTION_H_

#include "cc/ipc/compositor.mojom.h"
#include "cc/base/bulk_buffer_queue.h"
#include "cc/base/cc_export.h"
#include "mojo/public/cpp/bindings/interface_ptr.h"
#include "mojo/public/cpp/bindings/interface_request.h"


namespace cc {

struct CC_EXPORT ServiceConnection {
  ServiceConnection();
  ~ServiceConnection();

  BulkBufferWriter::AllocatorCallback shm_allocator;
  mojo::InterfacePtr<cc::mojom::ContentFrameSink> content_frame_sink;
  mojo::InterfaceRequest<cc::mojom::ContentFrameSinkClient> client_request;
};

}  // namespace cc

#endif  // CC_TREES_SERVICE_CONNECTION_H_
