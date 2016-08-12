// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/base/bulk_buffer_queue.h"

#include "base/memory/ptr_util.h"
#include "base/trace_event/trace_event.h"

namespace cc {
BulkBuffer::BulkBuffer() = default;
BulkBuffer::~BulkBuffer() = default;
BulkBuffer::BulkBuffer(const BulkBuffer&) = default;
BulkBuffer& BulkBuffer::operator=(const BulkBuffer&) = default;
BulkBuffer::BulkBuffer(BulkBuffer&&) = default;
BulkBuffer& BulkBuffer::operator=(BulkBuffer&&) = default;

BulkBufferWriter::BulkBufferWriter(size_t backing_size,
                                   AllocatorCallback allocator)
    : backing_size_(backing_size), allocator_(allocator) {}

BulkBufferWriter::~BulkBufferWriter() {}

void BulkBufferWriter::BeginBuffer() {
  DCHECK(!current_);
  if (!current_backing_)
    GetNewBacking();
  current_ = true;
  current_buffer_.backings = {current_backing_};
  current_buffer_.first_backing_begin = current_offset_;
  current_buffer_.last_backing_end = current_offset_;
}

void BulkBufferWriter::AppendToCurrentBuffer(size_t size, const void* data) {
  DCHECK(current_);
  DCHECK(current_backing_);
  const char* read_data = static_cast<const char*>(data);
  while (size) {
    size_t contiguous = std::min(size, backing_size_ - current_offset_);
    memcpy(current_memory_ + current_offset_, read_data, contiguous);
    current_offset_ += contiguous;
    read_data += contiguous;
    size -= contiguous;
    if (size)
      current_buffer_.backings.push_back(GetNewBacking());
  }
}

BulkBuffer BulkBufferWriter::EndBuffer() {
  DCHECK(current_);
  current_buffer_.last_backing_end = current_offset_;
  current_ = false;
  return std::move(current_buffer_);
}

void BulkBufferWriter::Flush(
    std::vector<uint32_t>* backings,
    std::vector<BulkBufferBackingHandle>* new_handles) {
  DCHECK(!current_);
  DCHECK(backings);
  DCHECK(new_handles);

  DCHECK(backings->empty());
  backings->swap(pending_backings_);
  for (uint32_t i = last_flushed_new_backing_ + 1; i <= next_backing_id_; ++i) {
    new_handles->emplace_back(
        i, base::SharedMemory::DuplicateHandle(backings_[i]->handle()));
  }
  last_flushed_new_backing_ = next_backing_id_;
  current_backing_ = 0;
  current_offset_ = 0;
}

void BulkBufferWriter::ReturnBackings(const std::vector<uint32_t>& backings) {
  free_backings_.insert(free_backings_.end(), backings.begin(), backings.end());
}

std::vector<uint32_t> BulkBufferWriter::Trim() {
  for (auto backing : free_backings_)
    backings_.erase(backing);
  return std::move(free_backings_);
}

uint32_t BulkBufferWriter::GetNewBacking() {
  uint32_t backing = 0;
  if (!free_backings_.empty()) {
    backing = free_backings_.back();
    free_backings_.pop_back();
    current_memory_ = static_cast<char*>(backings_[backing]->memory());
  } else {
    TRACE_EVENT0("cc", "BulkBufferWriter::GetNewBacking allocate");
    auto shm = allocator_.Run(backing_size_);
    if (!shm)
      return 0;
    if (!shm->Map(backing_size_))
      return 0;
    current_memory_ = static_cast<char*>(shm->memory());
    backing = ++next_backing_id_;
    backings_[backing] = std::move(shm);
  }
  pending_backings_.push_back(backing);
  current_backing_ = backing;
  current_offset_ = 0;
  return backing;
}

void BulkBufferReader::View::Read(size_t offset,
                                  size_t size,
                                  void* data) const {
  DCHECK_LE(offset + size, this->size());
  char* write_data = static_cast<char*>(data);
  while (size) {
    size_t contiguous = std::min(size, ContiguousSizeAt(offset));
    memcpy(write_data, DataAt(offset), contiguous);
    write_data += contiguous;
    offset += contiguous;
    size -= contiguous;
  }
}

size_t BulkBufferReader::View::ContiguousSizeAt(size_t offset) const {
  DCHECK_LT(offset, size());
  size_t tail_size = size() - offset;
  if (tail_size < buffer_.last_backing_end) {
    // Last backing, the tail of the buffer is contiguous.
    return tail_size;
  } else {
    // Otherwise only until the end of the backing.
    BackingOffset backing_offset = GetBackingOffset(offset);
    return reader_->backing_size_ - backing_offset.offset;
  }
}

const char* BulkBufferReader::View::DataAt(size_t offset) const {
  DCHECK_LT(offset, size());
  BackingOffset backing_offset = GetBackingOffset(offset);
  return reader_->GetBackingMemory(backing_offset.backing) +
         backing_offset.offset;
}

BulkBufferReader::BulkBufferReader(size_t backing_size)
    : backing_size_(backing_size) {}

BulkBufferReader::~BulkBufferReader() {}

bool BulkBufferReader::ImportBackings(
    std::vector<BulkBufferBackingHandle> new_handles) {
  bool success = true;
  for (auto handle : new_handles) {
    // Always make a SharedMemory to take ownership of the handle.
    auto shm = base::MakeUnique<base::SharedMemory>(handle.handle, false);
    if (!success)
      continue;
    success = shm->Map(backing_size_);
    if (!success)
      continue;
    backings_[handle.id] = std::move(shm);
  }
  return success;
}

void BulkBufferReader::DeleteBackings(const std::vector<uint32_t>& backings) {
  for (auto b : backings)
    backings_.erase(b);
}

}  // namespace cc
