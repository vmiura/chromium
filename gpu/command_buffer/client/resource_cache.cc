#include "resource_cache.h"

#include "base/hash.h"
#include "third_party/skia/include/core/SkTypes.h"

namespace gpu {

void ResourceCache::Key::init(void* nameSpace, size_t dataSize) {
  SkASSERT(SkAlign4(dataSize) == dataSize);

  // fCount32 and fHash are not hashed
  static const int kUnhashedLocal32s = 2; // fCache32 + fHash
  static const int kHashedLocal32s = sizeof(fNamespace) >> 2;
  static const int kLocal32s = kUnhashedLocal32s + kHashedLocal32s;

  static_assert(sizeof(Key) == (kLocal32s << 2), "unaccounted_key_locals");
  static_assert(sizeof(Key) == offsetof(Key, fNamespace) + sizeof(fNamespace),
               "namespace_field_must_be_last");

  fCount32 = SkToS32(kLocal32s + (dataSize >> 2));
  fNamespace = nameSpace;
  // skip unhashed fields when computing the hash
  fHash = base::Hash(reinterpret_cast<const char*>(this->as32() + kUnhashedLocal32s),
                     (fCount32 - kUnhashedLocal32s) << 2);
}

ResourceCache::ResourceCache() {
  fHead = nullptr;
  fTail = nullptr;
  fTotalBytesUsed = 0;
  fTotalByteLimit = 100 * 1024 * 1024;
  fCount = 0;
}

ResourceCache::~ResourceCache() {
  Rec* rec = fHead;
  while (rec) {
    Rec* next = rec->fNext;
    delete rec;
    rec = next;
  }
}

void ResourceCache::purgeAsNeeded(bool) {

}

void ResourceCache::release(Rec* rec) {
  Rec* prev = rec->fPrev;
  Rec* next = rec->fNext;

  if (!prev) {
      SkASSERT(fHead == rec);
      fHead = next;
  } else {
      prev->fNext = next;
  }

  if (!next) {
      fTail = prev;
  } else {
      next->fPrev = prev;
  }

  rec->fNext = rec->fPrev = nullptr;
}

void ResourceCache::moveToHead(Rec* rec) {
  if (fHead == rec) {
    return;
  }

  SkASSERT(fHead);
  SkASSERT(fTail);

  this->release(rec);

  fHead->fPrev = rec;
  rec->fNext = fHead;
  fHead = rec;
}

void ResourceCache::addToHead(Rec* rec) {
    rec->fPrev = nullptr;
    rec->fNext = fHead;
    if (fHead) {
        fHead->fPrev = rec;
    }
    fHead = rec;
    if (!fTail) {
        fTail = rec;
    }
    fTotalBytesUsed += rec->bytesUsed();
    fCount += 1;
}

bool ResourceCache::find(const Key& key, FindVisitor visitor, void* context) {
  auto it = hash_.find(&key);
  if (it != hash_.end()) {
    Rec* rec = it->second;
    if (visitor(*rec, context)) {
      this->moveToHead(rec);  // for our LRU
      return true;
    } else {
      this->remove(rec);  // stale
      return false;
    }
  }
  return false;
}

void ResourceCache::add(Rec* rec) {
  // See if we already have this key (racy inserts, etc.)
  auto it = hash_.find(&rec->getKey());
  if (it != hash_.end()) {
    delete rec;
    return;
  }

  this->addToHead(rec);
  hash_[&rec->getKey()] = rec;

  // since the new rec may push us over-budget, we perform a purge check now
  this->purgeAsNeeded();
}

void ResourceCache::remove(Rec* rec) {
  size_t used = rec->bytesUsed();
  SkASSERT(used <= fTotalBytesUsed);

  this->release(rec);
  hash_.erase(&rec->getKey());

  fTotalBytesUsed -= used;
  fCount -= 1;

  delete rec;
}

} // namespace gpu
