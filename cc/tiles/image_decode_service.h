// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_TILES_IMAGE_DECODE_SERVICE_H_
#define CC_TILES_IMAGE_DECODE_SERVICE_H_

#include "cc/base/cc_export.h"
#include "third_party/skia/include/core/SkImage.h"
#include "base/synchronization/lock.h"
#include "base/synchronization/condition_variable.h"
#include "base/threading/simple_thread.h"
#include "cc/base/unique_notifier.h"

#include <unordered_map>
#include <vector>
#include <deque>

namespace cc {

class CC_EXPORT ImageDecodeService {
 public:
  // TODO(hackathon): This shouldn't be a singleton.
  static ImageDecodeService* Current();

  ImageDecodeService();
  ~ImageDecodeService();

  // Blink accesses this to register things.
  void RegisterImage(sk_sp<SkImage> image);
  void UnregisterImage(uint32_t image_id);

  // Mojo accesses this?
  void DecodeImage(uint32_t image_id, void* buffer);

  // Called by the thread.
  void ProcessRequestQueue();
  // Call by origin thread.
  void ProcessResultQueue();

 private:
  bool DoDecodeImage(uint32_t image_id, void* buffer);
  void OnImageDecoded(uint32_t image_id, bool succeeded);

  base::Lock lock_;
  std::unordered_map<uint32_t, sk_sp<SkImage>> image_map_;

  base::ConditionVariable requests_cv_;
  std::deque<std::pair<uint32_t, void*>> image_decode_requests_;
  std::deque<std::pair<uint32_t, bool>> image_decode_results_;

  std::vector<std::unique_ptr<base::SimpleThread>> threads_;
  // TODO(hackathon): Hold on to this for lifetime issues?
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
  UniqueNotifier new_results_notifier_;

  DISALLOW_COPY_AND_ASSIGN(ImageDecodeService);
};

}  // namespace cc

#endif // CC_TILES_IMAGE_DECODE_SERVICE_H_
