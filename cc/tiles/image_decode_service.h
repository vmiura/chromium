// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_TILES_IMAGE_DECODE_SERVICE_H_
#define CC_TILES_IMAGE_DECODE_SERVICE_H_

#include "base/bind.h"
#include "cc/base/completion_event.h"
#include "base/synchronization/condition_variable.h"
#include "base/synchronization/lock.h"
#include "base/threading/simple_thread.h"
#include "base/threading/thread.h"
#include "cc/base/cc_export.h"
#include "cc/base/unique_notifier.h"
#include "cc/ipc/image_decode.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "third_party/skia/include/core/SkImage.h"

#include <unordered_map>
#include <vector>
#include <deque>

namespace cc {

class CC_EXPORT ImageDecodeService : public cc::mojom::ImageDecode {
 public:
  // TODO(hackathon): This shouldn't be a singleton.
  static ImageDecodeService* Current();

  ImageDecodeService();
  ~ImageDecodeService() override;

  // Mojo stuff
  void Bind(mojom::ImageDecodeRequest request);
  void DoBindOnServiceThread();
  void DoCloseMojoBinding(CompletionEvent* event);

  // mojom::ImageDecode:
  void DecodeImage(uint32_t unique_id,
                   uint64_t data,
                   const DecodeImageCallback& callback) override;

  // Blink accesses this to register things.
  void RegisterImage(sk_sp<SkImage> image);
  void UnregisterImage(uint32_t image_id);

  // Mojo accesses this?
  void DecodeImage(uint32_t image_id,
                   void* buffer,
                   const base::Closure& callback);

  // Called by the thread.
  void ProcessRequestQueue();
  // Call by origin thread.
  void ProcessResultQueue();

 private:
  static ImageDecodeService* s_service;

  struct ImageDecodeRequest {
    ImageDecodeRequest(uint32_t image_id,
                       void* buffer,
                       const base::Closure& callback);
    ~ImageDecodeRequest();

    uint32_t image_id;
    void* buffer;
    const base::Closure callback;

    DISALLOW_COPY_AND_ASSIGN(ImageDecodeRequest);
  };

  struct ImageDecodeResult {
    ImageDecodeResult(uint32_t image_id,
                      bool succeeded,
                      const base::Closure& callback);
    ~ImageDecodeResult();

    uint32_t image_id;
    bool succeeded;
    const base::Closure callback;

    DISALLOW_COPY_AND_ASSIGN(ImageDecodeResult);
  };

  bool DoDecodeImage(uint32_t image_id, void* buffer);
  void OnImageDecoded(uint32_t image_id,
                      bool succeeded,
                      const base::Closure& callback);

  base::Lock lock_;

  mojom::ImageDecodeRequest image_decode_request_;
  mojo::Binding<mojom::ImageDecode> image_decode_binding_;

  std::unordered_map<uint32_t, sk_sp<SkImage>> image_map_;

  base::ConditionVariable requests_cv_;
  base::Thread service_thread_;

  std::deque<std::unique_ptr<ImageDecodeRequest>> image_decode_requests_;
  std::deque<std::unique_ptr<ImageDecodeResult>> image_decode_results_;

  std::vector<std::unique_ptr<base::SimpleThread>> threads_;
  // TODO(hackathon): Hold on to this for lifetime issues?
  // scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
  std::unique_ptr<UniqueNotifier> new_results_notifier_;

  bool shutdown_ = false;

  DISALLOW_COPY_AND_ASSIGN(ImageDecodeService);
};

}  // namespace cc

#endif // CC_TILES_IMAGE_DECODE_SERVICE_H_
