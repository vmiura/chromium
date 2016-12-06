/*
 * Copyright 2016 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "cdl_picture_buffer.h"

#include "base/trace_event/trace_event.h"
#include "cdl_canvas.h"
#include "cdl_paint.h"
#include "cdl_picture.h"

#include "SkCanvas.h"
#include "SkData.h"
#include "SkDrawFilter.h"
#include "SkImageFilter.h"
#include "SkMath.h"
#include "SkPicture.h"
#include "SkRSXform.h"
#include "SkTextBlob.h"

#ifndef SKLITEDL_PAGE
#define SKLITEDL_PAGE 4096
#endif

// A stand-in for an optional SkRect which was not set, e.g. bounds for a
// saveLayer().
static const SkRect kUnset = {SK_ScalarInfinity, 0, 0, 0};
static const SkRect* maybe_unset(const SkRect& r) {
  return r.left() == SK_ScalarInfinity ? nullptr : &r;
}

// copy_v(dst, src,n, src,n, ...) copies an arbitrary number of typed srcs into
// dst.
static void copy_v(void* dst) {}

template <typename S, typename... Rest>
static void copy_v(void* dst, const S* src, int n, Rest&&... rest) {
  SkASSERTF(((uintptr_t)dst & (alignof(S) - 1)) == 0,
            "Expected %p to be aligned for at least %zu bytes.", dst,
            alignof(S));
  sk_careful_memcpy(dst, src, n * sizeof(S));
  copy_v(SkTAddOffset<void>(dst, n * sizeof(S)), std::forward<Rest>(rest)...);
}

// Helper for getting back at arrays which have been copy_v'd together after an
// Op.
template <typename D, typename T>
static D* pod(T* op, size_t offset = 0) {
  return SkTAddOffset<D>(op + 1, offset);
}

// Pre-cache lazy non-threadsafe fields on SkPath and/or SkMatrix.
static void make_threadsafe(SkPath* path, SkMatrix* matrix) {
  if (path) {
    path->updateBoundsCache();
  }
  if (matrix) {
    (void)matrix->getType();
  }
}

namespace {
#define TYPES(M)    \
  M(SetDrawFilter)  \
  M(Save)           \
  M(Restore)        \
  M(SaveLayer)      \
  M(Concat)         \
  M(SetMatrix)      \
  M(Translate)      \
  M(TranslateZ)     \
  M(ClipPath)       \
  M(ClipRect)       \
  M(ClipRRect)      \
  M(ClipRegion)     \
  M(DrawPaint)      \
  M(DrawPath)       \
  M(DrawRect)       \
  M(DrawOval)       \
  M(DrawRRect)      \
  M(DrawDRRect)     \
  M(DrawAnnotation) \
  M(DrawPicture)    \
  M(DrawImage)      \
  M(DrawImageRect)  \
  M(DrawText)       \
  M(DrawPosText)    \
  M(DrawTextBlob)   \
  M(DrawPoints)     \
  M(DrawRectX)      \
  M(DrawImageX)     \
  M(DrawImageRectX)

#define M(T) T,
enum class Type : uint8_t { TYPES(M) };
#undef M

struct Op {
  void makeThreadsafe() {}

  uint32_t type : 8;
  uint32_t skip : 24;
};
static_assert(sizeof(Op) == 4, "");

struct SetDrawFilter final : Op {
#ifdef SK_SUPPORT_LEGACY_DRAWFILTER
  static const auto kType = Type::SetDrawFilter;
  SetDrawFilter(SkDrawFilter* df) : drawFilter(sk_ref_sp(df)) {}
  sk_sp<SkDrawFilter> drawFilter;
#endif
  void draw(CdlCanvas* c, const SkMatrix&, CdlPictureBuffer::DrawContext&) {
#ifdef SK_SUPPORT_LEGACY_DRAWFILTER
    c->setDrawFilter(drawFilter.get());
#endif
  }
};

struct Save final : Op {
  static const auto kType = Type::Save;
  void draw(CdlCanvas* c, const SkMatrix&, CdlPictureBuffer::DrawContext&) {
    c->save();
  }
};
struct Restore final : Op {
  static const auto kType = Type::Restore;
  void draw(CdlCanvas* c, const SkMatrix&, CdlPictureBuffer::DrawContext&) {
    c->restore();
  }
};
struct SaveLayer final : Op {
  static const auto kType = Type::SaveLayer;
  SaveLayer(const SkRect* bounds,
            const SkPaint* paint,
            const SkImageFilter* backdrop,
            SkCanvas::SaveLayerFlags flags) {
    if (bounds) {
      this->bounds = *bounds;
    }
    if (paint) {
      this->paint = *paint;
    }
    this->backdrop = sk_ref_sp(backdrop);
    this->flags = flags;
  }
  SkRect bounds = kUnset;
  SkPaint paint;
  sk_sp<const SkImageFilter> backdrop;
  SkCanvas::SaveLayerFlags flags;
  void draw(CdlCanvas* c, const SkMatrix&, CdlPictureBuffer::DrawContext&) {
    c->saveLayer({maybe_unset(bounds), &paint, backdrop.get(), flags});
  }
};

struct Concat final : Op {
  static const auto kType = Type::Concat;
  Concat(const SkMatrix& matrix) : matrix(matrix) {}
  SkMatrix matrix;
  void draw(CdlCanvas* c, const SkMatrix&, CdlPictureBuffer::DrawContext&) {
    c->concat(matrix);
  }
  void makeThreadsafe() { make_threadsafe(nullptr, &matrix); }
};
struct SetMatrix final : Op {
  static const auto kType = Type::SetMatrix;
  SetMatrix(const SkMatrix& matrix) : matrix(matrix) {}
  SkMatrix matrix;
  void draw(CdlCanvas* c,
            const SkMatrix& original,
            CdlPictureBuffer::DrawContext&) {
    c->setMatrix(SkMatrix::Concat(original, matrix));
  }
  void makeThreadsafe() { make_threadsafe(nullptr, &matrix); }
};
struct Translate final : Op {
  static const auto kType = Type::Translate;
  Translate(SkScalar dx, SkScalar dy) : dx(dx), dy(dy) {}
  SkScalar dx, dy;
  void draw(CdlCanvas* c, const SkMatrix&, CdlPictureBuffer::DrawContext&) {
    c->translate(dx, dy);
  }
};
struct TranslateZ final : Op {
  static const auto kType = Type::TranslateZ;
  TranslateZ(SkScalar dz) : dz(dz) {}
  SkScalar dz;
  void draw(CdlCanvas* c, const SkMatrix&, CdlPictureBuffer::DrawContext&) {
#ifdef SK_EXPERIMENTAL_SHADOWING
    c->translateZ(dz);
#endif
  }
};

struct ClipPath final : Op {
  static const auto kType = Type::ClipPath;
  ClipPath(const SkPath& path, SkCanvas::ClipOp op, bool aa)
      : path(path), op(op), aa(aa) {}
  SkPath path;
  SkCanvas::ClipOp op;
  bool aa;
  void draw(CdlCanvas* c, const SkMatrix&, CdlPictureBuffer::DrawContext&) {
    c->clipPath(path, op, aa);
  }
  void makeThreadsafe() { make_threadsafe(&path, nullptr); }
};
struct ClipRect final : Op {
  static const auto kType = Type::ClipRect;
  ClipRect(const SkRect& rect, SkCanvas::ClipOp op, bool aa)
      : rect(rect), op(op), aa(aa) {}
  SkRect rect;
  SkCanvas::ClipOp op;
  bool aa;
  void draw(CdlCanvas* c, const SkMatrix&, CdlPictureBuffer::DrawContext&) {
    c->clipRect(rect, op, aa);
  }
};
struct ClipRRect final : Op {
  static const auto kType = Type::ClipRRect;
  ClipRRect(const SkRRect& rrect, SkCanvas::ClipOp op, bool aa)
      : rrect(rrect), op(op), aa(aa) {}
  SkRRect rrect;
  SkCanvas::ClipOp op;
  bool aa;
  void draw(CdlCanvas* c, const SkMatrix&, CdlPictureBuffer::DrawContext&) {
    c->clipRRect(rrect, op, aa);
  }
};
struct ClipRegion final : Op {
  static const auto kType = Type::ClipRegion;
  ClipRegion(const SkRegion& region, SkCanvas::ClipOp op)
      : region(region), op(op) {}
  SkRegion region;
  SkCanvas::ClipOp op;
  void draw(CdlCanvas* c, const SkMatrix&, CdlPictureBuffer::DrawContext&) {
    c->clipRegion(region, op);
  }
};

struct DrawPaint final : Op {
  static const auto kType = Type::DrawPaint;
  DrawPaint(const SkPaint& paint) : paint(paint) {}
  SkPaint paint;
  void draw(CdlCanvas* c, const SkMatrix&, CdlPictureBuffer::DrawContext&) {
    c->drawPaint(paint);
  }
};
struct DrawPath final : Op {
  static const auto kType = Type::DrawPath;
  DrawPath(const SkPath& path, const SkPaint& paint)
      : path(path), paint(paint) {}
  SkPath path;
  SkPaint paint;
  void draw(CdlCanvas* c, const SkMatrix&, CdlPictureBuffer::DrawContext&) {
    c->drawPath(path, paint);
  }
  void makeThreadsafe() { make_threadsafe(&path, nullptr); }
};
struct DrawRect final : Op {
  static const auto kType = Type::DrawRect;
  DrawRect(const SkRect& rect, const SkPaint& paint)
      : rect(rect), paint(paint) {}
  SkRect rect;
  SkPaint paint;
  void draw(CdlCanvas* c, const SkMatrix&, CdlPictureBuffer::DrawContext&) {
    c->drawRect(rect, paint);
  }
};

struct DrawRectX final : Op {
  static const auto kType = Type::DrawRectX;
  DrawRectX(const SkRect& rect, const CdlPaint& paint)
      : rect(rect), paint(paint) {}
  SkRect rect;
  CdlPaint paint;
  void draw(CdlCanvas* c, const SkMatrix&, CdlPictureBuffer::DrawContext&) {
    c->drawRect(rect, paint);
  }
};

struct DrawOval final : Op {
  static const auto kType = Type::DrawOval;
  DrawOval(const SkRect& oval, const SkPaint& paint)
      : oval(oval), paint(paint) {}
  SkRect oval;
  SkPaint paint;
  void draw(CdlCanvas* c, const SkMatrix&, CdlPictureBuffer::DrawContext&) {
    c->drawOval(oval, paint);
  }
};

struct DrawRRect final : Op {
  static const auto kType = Type::DrawRRect;
  DrawRRect(const SkRRect& rrect, const SkPaint& paint)
      : rrect(rrect), paint(paint) {}
  SkRRect rrect;
  SkPaint paint;
  void draw(CdlCanvas* c, const SkMatrix&, CdlPictureBuffer::DrawContext&) {
    c->drawRRect(rrect, paint);
  }
};
struct DrawDRRect final : Op {
  static const auto kType = Type::DrawDRRect;
  DrawDRRect(const SkRRect& outer, const SkRRect& inner, const SkPaint& paint)
      : outer(outer), inner(inner), paint(paint) {}
  SkRRect outer, inner;
  SkPaint paint;
  void draw(CdlCanvas* c, const SkMatrix&, CdlPictureBuffer::DrawContext&) {
    c->drawDRRect(outer, inner, paint);
  }
};

struct DrawAnnotation final : Op {
  static const auto kType = Type::DrawAnnotation;
  DrawAnnotation(const SkRect& rect, SkData* value)
      : rect(rect), value(sk_ref_sp(value)) {}
  SkRect rect;
  sk_sp<SkData> value;
  void draw(CdlCanvas* c, const SkMatrix&, CdlPictureBuffer::DrawContext&) {
    // TODO(cdl)
    // c->drawAnnotation(rect, pod<char>(this), value.get());
  }
};

struct DrawPicture final : Op {
  static const auto kType = Type::DrawPicture;
  DrawPicture(const CdlPicture* picture,
              const SkMatrix* matrix,
              const SkPaint* paint)
      : picture(sk_ref_sp(picture)) {
    if (matrix) {
      this->matrix = *matrix;
    }
    if (paint) {
      this->paint = *paint;
      has_paint = true;
    }
  }
  sk_sp<const CdlPicture> picture;
  SkMatrix matrix = SkMatrix::I();
  SkPaint paint;
  bool has_paint = false;  // TODO: why is a default paint not the same?
  void draw(CdlCanvas* c, const SkMatrix&, CdlPictureBuffer::DrawContext&) {
    c->drawPicture(picture.get(), &matrix, has_paint ? &paint : nullptr);
  }
  void makeThreadsafe() { make_threadsafe(nullptr, &matrix); }
};

struct DrawImage final : Op {
  static const auto kType = Type::DrawImage;
  DrawImage(sk_sp<const SkImage>&& image,
            SkScalar x,
            SkScalar y,
            const SkPaint* paint)
      : image(std::move(image)), x(x), y(y) {
    if (paint) {
      this->paint = *paint;
    }
  }
  sk_sp<const SkImage> image;
  SkScalar x, y;
  SkPaint paint;
  void draw(CdlCanvas* c, const SkMatrix&, CdlPictureBuffer::DrawContext&) {
    c->drawImage(image.get(), x, y, &paint);
  }
};
struct DrawImageX final : Op {
  static const auto kType = Type::DrawImage;
  DrawImageX(sk_sp<const SkImage>&& image,
             SkScalar x,
             SkScalar y,
             const CdlPaint& paint)
      : image(std::move(image)), x(x), y(y), paint(paint) {}
  sk_sp<const SkImage> image;
  SkScalar x, y;
  CdlPaint paint;
  void draw(CdlCanvas* c, const SkMatrix&, CdlPictureBuffer::DrawContext&) {
    SkPaint pt = paint.toSkPaint();
    c->drawImage(image.get(), x, y, &pt);
  }
};

struct DrawImageRect final : Op {
  static const auto kType = Type::DrawImageRect;
  DrawImageRect(sk_sp<const SkImage>&& image,
                const SkRect* src,
                const SkRect& dst,
                const SkPaint* paint,
                SkCanvas::SrcRectConstraint constraint)
      : image(std::move(image)), dst(dst), constraint(constraint) {
    this->src = src ? *src : SkRect::MakeIWH(image->width(), image->height());
    if (paint) {
      this->paint = *paint;
    }
  }
  sk_sp<const SkImage> image;
  SkRect src, dst;
  SkPaint paint;
  SkCanvas::SrcRectConstraint constraint;
  void draw(CdlCanvas* c, const SkMatrix&, CdlPictureBuffer::DrawContext&) {
    c->drawImageRect(image.get(), src, dst, &paint, constraint);
  }
};
struct DrawImageRectX final : Op {
  static const auto kType = Type::DrawImageRect;
  DrawImageRectX(sk_sp<const SkImage>&& image,
                 const SkRect* src,
                 const SkRect& dst,
                 const CdlPaint& paint,
                 SkCanvas::SrcRectConstraint constraint)
      : image(std::move(image)),
        dst(dst),
        paint(paint),
        constraint(constraint) {
    this->src = src ? *src : SkRect::MakeIWH(image->width(), image->height());
  }
  sk_sp<const SkImage> image;
  SkRect src, dst;
  CdlPaint paint;
  SkCanvas::SrcRectConstraint constraint;
  void draw(CdlCanvas* c, const SkMatrix&, CdlPictureBuffer::DrawContext&) {
    SkPaint pt = paint.toSkPaint();
    c->drawImageRect(image.get(), src, dst, &pt, constraint);
  }
};

struct DrawText final : Op {
  static const auto kType = Type::DrawText;
  DrawText(size_t bytes, SkScalar x, SkScalar y, const SkPaint& paint)
      : bytes(bytes), x(x), y(y), paint(paint) {}
  size_t bytes;
  SkScalar x, y;
  SkPaint paint;
  void draw(CdlCanvas* c, const SkMatrix&, CdlPictureBuffer::DrawContext&) {
    c->drawText(pod<void>(this), bytes, x, y, paint);
  }
};
struct DrawPosText final : Op {
  static const auto kType = Type::DrawPosText;
  DrawPosText(size_t bytes, const SkPaint& paint, int n)
      : bytes(bytes), paint(paint), n(n) {}
  size_t bytes;
  SkPaint paint;
  int n;
  void draw(CdlCanvas* c, const SkMatrix&, CdlPictureBuffer::DrawContext&) {
    auto points = pod<SkPoint>(this);
    auto text = pod<void>(this, n * sizeof(SkPoint));
    c->drawPosText(text, bytes, points, paint);
  }
};

struct DrawTextBlob final : Op {
  static const auto kType = Type::DrawTextBlob;
  DrawTextBlob(const SkTextBlob* blob,
               SkScalar x,
               SkScalar y,
               const SkPaint& paint)
      : blob(sk_ref_sp(blob)), x(x), y(y), paint(paint) {}
  sk_sp<const SkTextBlob> blob;
  SkScalar x, y;
  SkPaint paint;
  void draw(CdlCanvas* c, const SkMatrix&, CdlPictureBuffer::DrawContext& dc) {
    c->drawTextBlob(blob.get(), x, y, paint);
  }
};

struct DrawPoints final : Op {
  static const auto kType = Type::DrawPoints;
  DrawPoints(SkCanvas::PointMode mode, size_t count, const SkPaint& paint)
      : mode(mode), count(count), paint(paint) {}
  SkCanvas::PointMode mode;
  size_t count;
  SkPaint paint;
  void draw(CdlCanvas* c, const SkMatrix&, CdlPictureBuffer::DrawContext&) {
    c->drawPoints(mode, count, pod<SkPoint>(this), paint);
  }
};

}  // anon namespace

template <typename T, typename... Args>
void* CdlPictureBuffer::push(size_t pod, Args&&... args) {
  size_t skip = SkAlignPtr(sizeof(T) + pod);
  SkASSERT(skip < (1 << 24));
  if (fUsed + skip > fReserved) {
    static_assert(SkIsPow2(SKLITEDL_PAGE),
                  "This math needs updating for non-pow2.");
    // Next greater multiple of SKLITEDL_PAGE.
    fReserved = (fUsed + skip + SKLITEDL_PAGE) & ~(SKLITEDL_PAGE - 1);
    fBytes.realloc(fReserved);
  }
  SkASSERT(fUsed + skip <= fReserved);
  auto op = (T*)(fBytes.get() + fUsed);
  fUsed += skip;
  new (op) T{std::forward<Args>(args)...};
  op->type = (uint32_t)T::kType;
  op->skip = skip;
  return op + 1;
}

template <typename Fn, typename... Args>
inline void CdlPictureBuffer::map(const Fn fns[],
                                  int start_offset,
                                  int end_offset,
                                  Args... args) {
  auto start = fBytes.get() + start_offset;
  auto end = fBytes.get() + end_offset;
  for (uint8_t* ptr = start; ptr < end;) {
    auto op = (Op*)ptr;
    auto type = op->type;
    auto skip = op->skip;
    if (auto fn = fns[type]) {  // We replace no-op functions with nullptrs
      fn(op, args...);          // to avoid the overhead of a pointless call.
    }
    ptr += skip;
  }
}

#ifdef SK_SUPPORT_LEGACY_DRAWFILTER
void CdlPictureBuffer::setDrawFilter(SkDrawFilter* df) {
  this->push<SetDrawFilter>(0, df);
}
#endif

void CdlPictureBuffer::save() {
  this->push<Save>(0);
}
void CdlPictureBuffer::restore() {
  this->push<Restore>(0);
}
void CdlPictureBuffer::saveLayer(const SkRect* bounds,
                                 const SkPaint* paint,
                                 const SkImageFilter* backdrop,
                                 SkCanvas::SaveLayerFlags flags) {
  this->push<SaveLayer>(0, bounds, paint, backdrop, flags);
}

void CdlPictureBuffer::concat(const SkMatrix& matrix) {
  this->push<Concat>(0, matrix);
}
void CdlPictureBuffer::setMatrix(const SkMatrix& matrix) {
  this->push<SetMatrix>(0, matrix);
}
void CdlPictureBuffer::translate(SkScalar dx, SkScalar dy) {
  this->push<Translate>(0, dx, dy);
}
void CdlPictureBuffer::translateZ(SkScalar dz) {
  this->push<TranslateZ>(0, dz);
}

void CdlPictureBuffer::clipPath(const SkPath& path,
                                SkCanvas::ClipOp op,
                                bool aa) {
  this->push<ClipPath>(0, path, op, aa);
}
void CdlPictureBuffer::clipRect(const SkRect& rect,
                                SkCanvas::ClipOp op,
                                bool aa) {
  this->push<ClipRect>(0, rect, op, aa);
}
void CdlPictureBuffer::clipRRect(const SkRRect& rrect,
                                 SkCanvas::ClipOp op,
                                 bool aa) {
  this->push<ClipRRect>(0, rrect, op, aa);
}
void CdlPictureBuffer::clipRegion(const SkRegion& region, SkCanvas::ClipOp op) {
  this->push<ClipRegion>(0, region, op);
}

void CdlPictureBuffer::drawPaint(const SkPaint& paint) {
  this->push<DrawPaint>(0, paint);
}
void CdlPictureBuffer::drawPath(const SkPath& path, const SkPaint& paint) {
  this->push<DrawPath>(0, path, paint);
}
void CdlPictureBuffer::drawRect(const SkRect& rect, const SkPaint& paint) {
  this->push<DrawRect>(0, rect, paint);
}
void CdlPictureBuffer::drawRect(const SkRect& rect, const CdlPaint& paint) {
  this->push<DrawRectX>(0, rect, paint);
}

void CdlPictureBuffer::drawOval(const SkRect& oval, const SkPaint& paint) {
  this->push<DrawOval>(0, oval, paint);
}

void CdlPictureBuffer::drawRRect(const SkRRect& rrect, const SkPaint& paint) {
  this->push<DrawRRect>(0, rrect, paint);
}
void CdlPictureBuffer::drawDRRect(const SkRRect& outer,
                                  const SkRRect& inner,
                                  const SkPaint& paint) {
  this->push<DrawDRRect>(0, outer, inner, paint);
}

void CdlPictureBuffer::drawAnnotation(const SkRect& rect,
                                      const char* key,
                                      SkData* value) {
  size_t bytes = strlen(key) + 1;
  void* pod = this->push<DrawAnnotation>(bytes, rect, value);
  copy_v(pod, key, bytes);
}

void CdlPictureBuffer::drawPicture(const CdlPicture* picture,
                                   const SkMatrix* matrix,
                                   const SkPaint* paint) {
  this->push<DrawPicture>(0, picture, matrix, paint);
}

void CdlPictureBuffer::drawImage(sk_sp<const SkImage> image,
                                 SkScalar x,
                                 SkScalar y,
                                 const SkPaint* paint) {
  this->push<DrawImage>(0, std::move(image), x, y, paint);
}
void CdlPictureBuffer::drawImage(sk_sp<const SkImage> image,
                                 SkScalar x,
                                 SkScalar y,
                                 const CdlPaint& paint) {
  this->push<DrawImageX>(0, std::move(image), x, y, paint);
}

void CdlPictureBuffer::drawImageRect(sk_sp<const SkImage> image,
                                     const SkRect* src,
                                     const SkRect& dst,
                                     const SkPaint* paint,
                                     SkCanvas::SrcRectConstraint constraint) {
  this->push<DrawImageRect>(0, std::move(image), src, dst, paint, constraint);
}
void CdlPictureBuffer::drawImageRect(sk_sp<const SkImage> image,
                                     const SkRect* src,
                                     const SkRect& dst,
                                     const CdlPaint& paint,
                                     SkCanvas::SrcRectConstraint constraint) {
  this->push<DrawImageRectX>(0, std::move(image), src, dst, paint, constraint);
}

void CdlPictureBuffer::drawText(const void* text,
                                size_t bytes,
                                SkScalar x,
                                SkScalar y,
                                const SkPaint& paint) {
  void* pod = this->push<DrawText>(bytes, bytes, x, y, paint);
  copy_v(pod, (const char*)text, bytes);
}
void CdlPictureBuffer::drawPosText(const void* text,
                                   size_t bytes,
                                   const SkPoint pos[],
                                   const SkPaint& paint) {
  int n = paint.countText(text, bytes);
  void* pod =
      this->push<DrawPosText>(n * sizeof(SkPoint) + bytes, bytes, paint, n);
  copy_v(pod, pos, n, (const char*)text, bytes);
}

void CdlPictureBuffer::drawTextBlob(const SkTextBlob* blob,
                                    SkScalar x,
                                    SkScalar y,
                                    const SkPaint& paint) {
  this->push<DrawTextBlob>(0, blob, x, y, paint);
}

void CdlPictureBuffer::drawPoints(SkCanvas::PointMode mode,
                                  size_t count,
                                  const SkPoint points[],
                                  const SkPaint& paint) {
  void* pod =
      this->push<DrawPoints>(count * sizeof(SkPoint), mode, count, paint);
  copy_v(pod, points, count);
}

typedef void (*draw_fn)(void*,
                        CdlCanvas*,
                        const SkMatrix&,
                        CdlPictureBuffer::DrawContext&);
typedef void (*void_fn)(void*);

// All ops implement draw().
#define M(T)                                           \
  [](void* op, CdlCanvas* c, const SkMatrix& original, \
     CdlPictureBuffer::DrawContext& dc) { ((T*)op)->draw(c, original, dc); },
static const draw_fn draw_fns[] = {TYPES(M)};
#undef M

#define M(T) [](void* op) { ((T*)op)->makeThreadsafe(); },
static const void_fn make_threadsafe_fns[] = {TYPES(M)};
#undef M

// Older libstdc++ has pre-standard std::has_trivial_destructor.
#if defined(__GLIBCXX__) && (__GLIBCXX__ < 20130000)
template <typename T>
using can_skip_destructor = std::has_trivial_destructor<T>;
#else
template <typename T>
using can_skip_destructor = std::is_trivially_destructible<T>;
#endif

// Most state ops (matrix, clip, save, restore) have a trivial destructor.
#define M(T)                                                        \
  !can_skip_destructor<T>::value ? [](void* op) { ((T*)op)->~T(); } \
                                 : (void_fn) nullptr,
static const void_fn dtor_fns[] = {TYPES(M)};
#undef M

void CdlPictureBuffer::playback(CdlCanvas* canvas,
                                int start_offset,
                                int end_offset) {
  DrawContext dc;
  this->map(draw_fns, start_offset, end_offset, canvas,
            canvas->getTotalMatrix(), dc);
}

void CdlPictureBuffer::makeThreadsafe() {
  this->map(make_threadsafe_fns, 0, fUsed);
}

SkRect CdlPictureBuffer::getBounds() {
  return fBounds;
}

CdlPictureBuffer::CdlPictureBuffer(SkRect bounds)
    : fUsed(0), fReserved(0), fBounds(bounds) {}

CdlPictureBuffer::~CdlPictureBuffer() {
  this->reset(SkRect::MakeEmpty());
}

void CdlPictureBuffer::resetForNextPicture(SkRect bounds) {
  fBounds = bounds;
}

void CdlPictureBuffer::reset(SkRect bounds) {
  SkASSERT(this->unique());
  this->map(dtor_fns, 0, fUsed);

  // Leave fBytes and fReserved alone.
  fUsed = 0;
  fBounds = bounds;
}

void CdlPictureBuffer::drawAsLayer(CdlCanvas* canvas,
                                   const SkMatrix* matrix,
                                   const SkPaint* paint) {
  // TODO(cdl)
  /*
  auto fallback_plan = [&] {
    SkRect bounds = this->getBounds();
    canvas->saveLayer(&bounds, paint);
    this->draw(canvas, matrix);
    canvas->restore();
  };

  // TODO: single-draw specializations

  return fallback_plan();
  */
}

void CdlPictureBuffer::setBounds(const SkRect& bounds) {
  fBounds = bounds;
}
