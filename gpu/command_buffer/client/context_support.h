// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_COMMAND_BUFFER_CLIENT_CONTEXT_SUPPORT_H_
#define GPU_COMMAND_BUFFER_CLIENT_CONTEXT_SUPPORT_H_

#include <stdint.h>
#include <unordered_map>

#include "base/callback.h"
#include "third_party/skia/include/core/SkFlattenable.h"
#include "third_party/skia/src/core/SkDeduper.h"
#include "ui/gfx/overlay_transform.h"

class SkTextBlob;
class SkPath;
class SkResourceCache;

namespace gfx {
class Rect;
class RectF;
}

namespace gpu {

struct SyncToken;

class CdlResourceCache {
 public:
  struct Key {
    /** Key subclasses must call this after their own fields and data are initialized.
     *  All fields and data must be tightly packed.
     *  @param nameSpace must be unique per Key subclass.
     *  @param dataSize is size of fields and data of the subclass, must be a multiple of 4.
     */
    void init(void* nameSpace, size_t dataSize);

    /** Returns the size of this key. */
    size_t size() const {
        return fCount32 << 2;
    }

    void* getNamespace() const { return fNamespace; }

    // This is only valid after having called init().
    uint32_t hash() const { return fHash; }

    bool operator==(const Key& other) const {
      const uint32_t* a = this->as32();
      const uint32_t* b = other.as32();
      for (int i = 0; i < fCount32; ++i) {  // (This checks fCount == other.fCount first.)
        if (a[i] != b[i]) {
          return false;
        }
      }
      return true;
    }

  private:
    int32_t  fCount32;   // local + user contents count32
    uint32_t fHash;
    void*    fNamespace; // A unique namespace tag. This is hashed.
    /* uint32_t fContents32[] */

    const uint32_t* as32() const { return (const uint32_t*)this; }
  };

  struct Rec {
    typedef CdlResourceCache::Key Key;

    Rec() {}
    virtual ~Rec() {}

    uint32_t getHash() const { return this->getKey().hash(); }

    virtual const Key& getKey() const = 0;
    virtual size_t bytesUsed() const = 0;

    // for memory usage diagnostics
    virtual const char* getCategory() const = 0;

   private:
    Rec*    fNext;
    Rec*    fPrev;

    friend class CdlResourceCache;
  };

  CdlResourceCache();
  ~CdlResourceCache();

  typedef bool (*FindVisitor)(const Rec&, void* context);

  void moveToHead(Rec*);
  void addToHead(Rec*);
  void release(Rec*);
  void remove(Rec*);

  bool find(const Key&, FindVisitor, void* context);
  void add(Rec*);

  void purgeAsNeeded(bool forcePurge = false);

  size_t  fTotalBytesUsed;
  size_t  fTotalByteLimit;
  int     fCount;

  Rec*    fHead;
  Rec*    fTail;

  typedef const Key* KeyType;

  struct KeyHash {
    size_t operator()(const KeyType& key) const {
      return key->hash();
    }
  };

  struct KeyEqual {
    bool operator()( const KeyType& lhs, const KeyType& rhs ) const {
      return *lhs == *rhs;
    }
  };

  std::unordered_map<KeyType, Rec*, KeyHash, KeyEqual> hash_;
};

class ContextSupport : public SkDeduper {
 public:
  // Runs |callback| when a sync token is signalled.
  virtual void SignalSyncToken(const SyncToken& sync_token,
                               const base::Closure& callback) = 0;

  // Runs |callback| when a query created via glCreateQueryEXT() has cleared
  // passed the glEndQueryEXT() point.
  virtual void SignalQuery(uint32_t query, const base::Closure& callback) = 0;

  // Indicates whether the context should aggressively free allocated resources.
  // If set to true, the context will purge all temporary resources when
  // flushed.
  virtual void SetAggressivelyFreeResources(
      bool aggressively_free_resources) = 0;

  virtual void Swap() = 0;
  virtual void SwapWithDamage(const gfx::Rect& damage) = 0;
  virtual void PartialSwapBuffers(const gfx::Rect& sub_buffer) = 0;
  virtual void CommitOverlayPlanes() = 0;

  // Schedule a texture to be presented as an overlay synchronously with the
  // primary surface during the next buffer swap or CommitOverlayPlanes.
  // This method is not stateful and needs to be re-scheduled every frame.
  virtual void ScheduleOverlayPlane(int plane_z_order,
                                    gfx::OverlayTransform plane_transform,
                                    unsigned overlay_texture_id,
                                    const gfx::Rect& display_bounds,
                                    const gfx::RectF& uv_rect) = 0;

  // Returns an ID that can be used to globally identify the share group that
  // this context's resources belong to.
  virtual uint64_t ShareGroupTracingGUID() const = 0;

  // Sets a callback to be run when an error occurs.
  virtual void SetErrorMessageCallback(
      const base::Callback<void(const char*, int32_t)>& callback) = 0;


  // CDL HACKING ///////////////////////////////////////////////////////////////
  virtual int findOrDefineTextBlob(const SkTextBlob*) = 0;
  virtual int findOrDefinePath(const SkPath*) = 0;
  virtual CdlResourceCache* resource_cache() = 0;
  //////////////////////////////////////////////////////////////////////////////


 protected:
  ContextSupport() {}
  ~ContextSupport() override {}
};

}

#endif  // GPU_COMMAND_BUFFER_CLIENT_CONTEXT_SUPPORT_H_
