// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_COMMAND_BUFFER_CLIENT_RESOURCE_CACHE_H_
#define GPU_COMMAND_BUFFER_CLIENT_RESOURCE_CACHE_H_

#include <cstddef>
#include <stdint.h>
#include <unordered_map>

#include "gpu/gpu_export.h"

class SkTextBlob;
class SkPath;

namespace gpu {

class GPU_EXPORT ResourceCache {
 public:
  struct Key {
    /** Key subclasses must call this after their own fields and data are
     * initialized.
     *  All fields and data must be tightly packed.
     *  @param nameSpace must be unique per Key subclass.
     *  @param dataSize is size of fields and data of the subclass, must be a
     * multiple of 4.
     */
    void init(void* nameSpace, size_t dataSize);

    /** Returns the size of this key. */
    size_t size() const { return fCount32 << 2; }

    void* getNamespace() const { return fNamespace; }

    // This is only valid after having called init().
    uint32_t hash() const { return fHash; }

    bool operator==(const Key& other) const {
      const uint32_t* a = this->as32();
      const uint32_t* b = other.as32();
      for (int i = 0; i < fCount32;
           ++i) {  // (This checks fCount == other.fCount first.)
        if (a[i] != b[i]) {
          return false;
        }
      }
      return true;
    }

   private:
    int32_t fCount32;  // local + user contents count32
    uint32_t fHash;
    void* fNamespace;  // A unique namespace tag. This is hashed.
    /* uint32_t fContents32[] */

    const uint32_t* as32() const { return (const uint32_t*)this; }
  };

  struct Rec {
    typedef ResourceCache::Key Key;

    Rec() {}
    virtual ~Rec() {}

    uint32_t getHash() const { return this->getKey().hash(); }

    virtual const Key& getKey() const = 0;
    virtual size_t bytesUsed() const = 0;

    // for memory usage diagnostics
    virtual const char* getCategory() const = 0;

   private:
    Rec* fNext;
    Rec* fPrev;

    friend class ResourceCache;
  };

  ResourceCache();
  ~ResourceCache();

  typedef bool (*FindVisitor)(const Rec&, void* context);

  void moveToHead(Rec*);
  void addToHead(Rec*);
  void release(Rec*);
  void remove(Rec*);

  bool find(const Key&, FindVisitor, void* context);
  void add(Rec*);

  void purgeAsNeeded(bool forcePurge = false);

  size_t fTotalBytesUsed;
  size_t fTotalByteLimit;
  int fCount;

  Rec* fHead;
  Rec* fTail;

  typedef const Key* KeyType;

  struct KeyHash {
    size_t operator()(const KeyType& key) const { return key->hash(); }
  };

  struct KeyEqual {
    bool operator()(const KeyType& lhs, const KeyType& rhs) const {
      return *lhs == *rhs;
    }
  };

  std::unordered_map<KeyType, Rec*, KeyHash, KeyEqual> hash_;
};
}

#endif  // GPU_COMMAND_BUFFER_CLIENT_RESOURCE_CACHE_H_
