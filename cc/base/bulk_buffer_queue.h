// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_BASE_BULK_BUFFER_QUEUE_H_
#define CC_BASE_BULK_BUFFER_QUEUE_H_

#include <deque>
#include <memory>
#include <unordered_map>
#include <vector>

#include "base/callback.h"
#include "base/memory/shared_memory.h"
#include "cc/base/cc_export.h"

namespace cc {

struct CC_EXPORT BulkBuffer {
  BulkBuffer();
  ~BulkBuffer();
  BulkBuffer(const BulkBuffer&);
  BulkBuffer& operator=(const BulkBuffer&);
  BulkBuffer(BulkBuffer&&);
  BulkBuffer& operator=(BulkBuffer&&);

  std::vector<uint32_t> backings;
  size_t first_backing_begin;
  size_t last_backing_end;
};

struct CC_EXPORT BulkBufferBackingHandle {
  BulkBufferBackingHandle(uint32_t id, base::SharedMemoryHandle handle)
      : id(id), handle(handle) {}
  uint32_t id;
  base::SharedMemoryHandle handle;
};

class CC_EXPORT BulkBufferWriter {
 public:
  static const size_t kDefaultBackingSize = 2<<20;
  using AllocatorCallback =
      base::Callback<std::unique_ptr<base::SharedMemory>(size_t)>;
  explicit BulkBufferWriter(size_t backing_size,
                            AllocatorCallback allocator);
  ~BulkBufferWriter();

  BulkBuffer MakeBuffer(size_t size, const void* data) {
    BeginBuffer();
    AppendToCurrentBuffer(size, data);
    return EndBuffer();
  }

  void BeginBuffer();
  void AppendToCurrentBuffer(size_t size, const void* data);
  BulkBuffer EndBuffer();

  void Flush(std::vector<uint32_t>* backings,
             std::vector<BulkBufferBackingHandle>* new_handles);
  void ReturnBackings(const std::vector<uint32_t>& backings);

  // Return unused backings.
  std::vector<uint32_t> Trim();

 private:
  uint32_t GetNewBacking();

  size_t backing_size_;
  AllocatorCallback allocator_;

  bool current_ = false;
  BulkBuffer current_buffer_;
  uint32_t current_backing_ = 0;
  size_t current_offset_ = 0;
  char* current_memory_ = nullptr;

  uint32_t next_backing_id_ = 0;
  uint32_t last_flushed_new_backing_ = 0;
  std::unordered_map<uint32_t, std::unique_ptr<base::SharedMemory>> backings_;
  std::vector<uint32_t> free_backings_;
  std::vector<uint32_t> pending_backings_;

  DISALLOW_COPY_AND_ASSIGN(BulkBufferWriter);
};

class CC_EXPORT BulkBufferReader {
 public:
  class View {
   public:
    View(const BulkBufferReader* reader, BulkBuffer buffer)
        : reader_(reader), buffer_(std::move(buffer)) {}

    size_t size() const {
      DCHECK((buffer_.backings.size() > 1) ||
             (buffer_.first_backing_begin <= buffer_.last_backing_end));
      return (buffer_.backings.size() - 1) * reader_->backing_size_ +
             buffer_.last_backing_end - buffer_.first_backing_begin;
    }

    void Read(size_t offset, size_t size, void* data) const;
    size_t ContiguousSizeAt(size_t offset) const;
    const char* DataAt(size_t offset) const;

   private:
    struct BackingOffset {
      uint32_t backing;
      size_t offset;
    };

    BackingOffset GetBackingOffset(size_t offset) const {
      DCHECK_LT(offset, size());
      size_t backing_size = reader_->backing_size_;
      size_t linear = offset + buffer_.first_backing_begin;
      return BackingOffset{buffer_.backings[linear / backing_size],
                           linear % backing_size};
    }

    const BulkBufferReader* reader_;
    const BulkBuffer buffer_;
  };

  explicit BulkBufferReader(size_t backing_size);
  ~BulkBufferReader();

  bool ImportBackings(std::vector<BulkBufferBackingHandle> new_handles);
  void DeleteBackings(const std::vector<uint32_t>& backings);

  View MakeView(BulkBuffer buffer) const {
    return View(this, std::move(buffer));
  }

 private:
  const char* GetBackingMemory(uint32_t backing) const {
    DCHECK(backings_.at(backing));
    return static_cast<char*>(backings_.at(backing)->memory());
  }

  size_t backing_size_;

  std::unordered_map<uint32_t, std::unique_ptr<base::SharedMemory>> backings_;

  DISALLOW_COPY_AND_ASSIGN(BulkBufferReader);
};

} // cc

#endif  // CC_BASE_BULK_BUFFER_QUEUE_H_
