/*
 * Copyright 2016 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "cdl_lite_dl.h"
#include "cdl_lite_recorder.h"
#include "cdl_paint.h"

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
#define TYPES(M)                                                             \
  M(SetDrawFilter)                                                           \
  M(Save)                                                                    \
  M(Restore) M(SaveLayer) M(Concat) M(SetMatrix) M(Translate) M(TranslateZ)  \
      M(ClipPath) M(ClipRect) M(ClipRRect) M(ClipRegion) M(DrawPaint)        \
          M(DrawPath) M(DrawRect) M(DrawRegion) M(DrawOval) M(DrawArc)       \
              M(DrawRRect) M(DrawDRRect) M(DrawAnnotation) M(DrawDrawable)   \
                  M(DrawPicture) M(DrawShadowedPicture) M(DrawImage)         \
                      M(DrawImageNine) M(DrawImageRect) M(DrawImageLattice)  \
                          M(DrawText) M(DrawPosText) M(DrawPosTextH)         \
                              M(DrawTextOnPath) M(DrawTextRSXform)           \
                                  M(DrawTextBlob) M(DrawPatch) M(DrawPoints) \
                                      M(DrawVertices) M(DrawAtlas)           \
                                          M(DrawRectX) M(DrawImageX)         \
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
  void draw(SkCanvas* c, const SkMatrix&, CdlLiteDL::DrawContext&) {
#ifdef SK_SUPPORT_LEGACY_DRAWFILTER
    c->setDrawFilter(drawFilter.get());
#endif
  }
};

struct Save final : Op {
  static const auto kType = Type::Save;
  void draw(SkCanvas* c, const SkMatrix&, CdlLiteDL::DrawContext&) {
    c->save();
  }
};
struct Restore final : Op {
  static const auto kType = Type::Restore;
  void draw(SkCanvas* c, const SkMatrix&, CdlLiteDL::DrawContext&) {
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
  void draw(SkCanvas* c, const SkMatrix&, CdlLiteDL::DrawContext&) {
    c->saveLayer({maybe_unset(bounds), &paint, backdrop.get(), flags});
  }
};

struct Concat final : Op {
  static const auto kType = Type::Concat;
  Concat(const SkMatrix& matrix) : matrix(matrix) {}
  SkMatrix matrix;
  void draw(SkCanvas* c, const SkMatrix&, CdlLiteDL::DrawContext&) {
    c->concat(matrix);
  }
  void makeThreadsafe() { make_threadsafe(nullptr, &matrix); }
};
struct SetMatrix final : Op {
  static const auto kType = Type::SetMatrix;
  SetMatrix(const SkMatrix& matrix) : matrix(matrix) {}
  SkMatrix matrix;
  void draw(SkCanvas* c, const SkMatrix& original, CdlLiteDL::DrawContext&) {
    c->setMatrix(SkMatrix::Concat(original, matrix));
  }
  void makeThreadsafe() { make_threadsafe(nullptr, &matrix); }
};
struct Translate final : Op {
  static const auto kType = Type::Translate;
  Translate(SkScalar dx, SkScalar dy) : dx(dx), dy(dy) {}
  SkScalar dx, dy;
  void draw(SkCanvas* c, const SkMatrix&, CdlLiteDL::DrawContext&) {
    c->translate(dx, dy);
  }
};
struct TranslateZ final : Op {
  static const auto kType = Type::TranslateZ;
  TranslateZ(SkScalar dz) : dz(dz) {}
  SkScalar dz;
  void draw(SkCanvas* c, const SkMatrix&, CdlLiteDL::DrawContext&) {
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
  void draw(SkCanvas* c, const SkMatrix&, CdlLiteDL::DrawContext&) {
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
  void draw(SkCanvas* c, const SkMatrix&, CdlLiteDL::DrawContext&) {
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
  void draw(SkCanvas* c, const SkMatrix&, CdlLiteDL::DrawContext&) {
    c->clipRRect(rrect, op, aa);
  }
};
struct ClipRegion final : Op {
  static const auto kType = Type::ClipRegion;
  ClipRegion(const SkRegion& region, SkCanvas::ClipOp op)
      : region(region), op(op) {}
  SkRegion region;
  SkCanvas::ClipOp op;
  void draw(SkCanvas* c, const SkMatrix&, CdlLiteDL::DrawContext&) {
    c->clipRegion(region, op);
  }
};

struct DrawPaint final : Op {
  static const auto kType = Type::DrawPaint;
  DrawPaint(const SkPaint& paint) : paint(paint) {}
  SkPaint paint;
  void draw(SkCanvas* c, const SkMatrix&, CdlLiteDL::DrawContext&) {
    c->drawPaint(paint);
  }
};
struct DrawPath final : Op {
  static const auto kType = Type::DrawPath;
  DrawPath(const SkPath& path, const SkPaint& paint)
      : path(path), paint(paint) {}
  SkPath path;
  SkPaint paint;
  void draw(SkCanvas* c, const SkMatrix&, CdlLiteDL::DrawContext&) {
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
  void draw(SkCanvas* c, const SkMatrix&, CdlLiteDL::DrawContext&) {
    c->drawRect(rect, paint);
  }
};

struct DrawRectX final : Op {
  static const auto kType = Type::DrawRectX;
  DrawRectX(const SkRect& rect, const CdlPaint& paint)
      : rect(rect), paint(paint) {}
  SkRect rect;
  CdlPaint paint;
  void draw(SkCanvas* c, const SkMatrix&, CdlLiteDL::DrawContext&) {
    c->drawRect(rect, paint.toSkPaint());
  }
};

struct DrawRegion final : Op {
  static const auto kType = Type::DrawRegion;
  DrawRegion(const SkRegion& region, const SkPaint& paint)
      : region(region), paint(paint) {}
  SkRegion region;
  SkPaint paint;
  void draw(SkCanvas* c, const SkMatrix&, CdlLiteDL::DrawContext&) {
    c->drawRegion(region, paint);
  }
};
struct DrawOval final : Op {
  static const auto kType = Type::DrawOval;
  DrawOval(const SkRect& oval, const SkPaint& paint)
      : oval(oval), paint(paint) {}
  SkRect oval;
  SkPaint paint;
  void draw(SkCanvas* c, const SkMatrix&, CdlLiteDL::DrawContext&) {
    c->drawOval(oval, paint);
  }
};
struct DrawArc final : Op {
  static const auto kType = Type::DrawArc;
  DrawArc(const SkRect& oval,
          SkScalar startAngle,
          SkScalar sweepAngle,
          bool useCenter,
          const SkPaint& paint)
      : oval(oval),
        startAngle(startAngle),
        sweepAngle(sweepAngle),
        useCenter(useCenter),
        paint(paint) {}
  SkRect oval;
  SkScalar startAngle;
  SkScalar sweepAngle;
  bool useCenter;
  SkPaint paint;
  void draw(SkCanvas* c, const SkMatrix&, CdlLiteDL::DrawContext&) {
    c->drawArc(oval, startAngle, sweepAngle, useCenter, paint);
  }
};
struct DrawRRect final : Op {
  static const auto kType = Type::DrawRRect;
  DrawRRect(const SkRRect& rrect, const SkPaint& paint)
      : rrect(rrect), paint(paint) {}
  SkRRect rrect;
  SkPaint paint;
  void draw(SkCanvas* c, const SkMatrix&, CdlLiteDL::DrawContext&) {
    c->drawRRect(rrect, paint);
  }
};
struct DrawDRRect final : Op {
  static const auto kType = Type::DrawDRRect;
  DrawDRRect(const SkRRect& outer, const SkRRect& inner, const SkPaint& paint)
      : outer(outer), inner(inner), paint(paint) {}
  SkRRect outer, inner;
  SkPaint paint;
  void draw(SkCanvas* c, const SkMatrix&, CdlLiteDL::DrawContext&) {
    c->drawDRRect(outer, inner, paint);
  }
};

struct DrawAnnotation final : Op {
  static const auto kType = Type::DrawAnnotation;
  DrawAnnotation(const SkRect& rect, SkData* value)
      : rect(rect), value(sk_ref_sp(value)) {}
  SkRect rect;
  sk_sp<SkData> value;
  void draw(SkCanvas* c, const SkMatrix&, CdlLiteDL::DrawContext&) {
    c->drawAnnotation(rect, pod<char>(this), value.get());
  }
};
struct DrawDrawable final : Op {
  static const auto kType = Type::DrawDrawable;
  DrawDrawable(SkDrawable* drawable, const SkMatrix* matrix)
      : drawable(sk_ref_sp(drawable)) {
    if (matrix) {
      this->matrix = *matrix;
    }
  }
  sk_sp<SkDrawable> drawable;
  sk_sp<const SkPicture> snapped;
  SkMatrix matrix = SkMatrix::I();
  void draw(SkCanvas* c, const SkMatrix&, CdlLiteDL::DrawContext&) {
    snapped ? c->drawPicture(snapped.get(), &matrix, nullptr)
            : c->drawDrawable(drawable.get(), &matrix);
  }
  void makeThreadsafe() {
    snapped.reset(drawable->newPictureSnapshot());
    make_threadsafe(nullptr, &matrix);
  }
};
struct DrawPicture final : Op {
  static const auto kType = Type::DrawPicture;
  DrawPicture(const SkPicture* picture,
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
  sk_sp<const SkPicture> picture;
  SkMatrix matrix = SkMatrix::I();
  SkPaint paint;
  bool has_paint = false;  // TODO: why is a default paint not the same?
  void draw(SkCanvas* c, const SkMatrix&, CdlLiteDL::DrawContext&) {
    c->drawPicture(picture.get(), &matrix, has_paint ? &paint : nullptr);
  }
  void makeThreadsafe() { make_threadsafe(nullptr, &matrix); }
};
struct DrawShadowedPicture final : Op {
  static const auto kType = Type::DrawShadowedPicture;
  DrawShadowedPicture(const SkPicture* picture,
                      const SkMatrix* matrix,
                      const SkPaint* paint,
                      const SkShadowParams& params)
      : picture(sk_ref_sp(picture)) {
    if (matrix) {
      this->matrix = *matrix;
    }
    if (paint) {
      this->paint = *paint;
    }
    this->params = params;
  }
  sk_sp<const SkPicture> picture;
  SkMatrix matrix = SkMatrix::I();
  SkPaint paint;
  SkShadowParams params;
  void draw(SkCanvas* c, const SkMatrix&, CdlLiteDL::DrawContext&) {
#ifdef SK_EXPERIMENTAL_SHADOWING
    c->drawShadowedPicture(picture.get(), &matrix, &paint, params);
#endif
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
  void draw(SkCanvas* c, const SkMatrix&, CdlLiteDL::DrawContext&) {
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
  void draw(SkCanvas* c, const SkMatrix&, CdlLiteDL::DrawContext&) {
    SkPaint pt = paint.toSkPaint();
    c->drawImage(image.get(), x, y, &pt);
  }
};

struct DrawImageNine final : Op {
  static const auto kType = Type::DrawImageNine;
  DrawImageNine(sk_sp<const SkImage>&& image,
                const SkIRect& center,
                const SkRect& dst,
                const SkPaint* paint)
      : image(std::move(image)), center(center), dst(dst) {
    if (paint) {
      this->paint = *paint;
    }
  }
  sk_sp<const SkImage> image;
  SkIRect center;
  SkRect dst;
  SkPaint paint;
  void draw(SkCanvas* c, const SkMatrix&, CdlLiteDL::DrawContext&) {
    c->drawImageNine(image.get(), center, dst, &paint);
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
  void draw(SkCanvas* c, const SkMatrix&, CdlLiteDL::DrawContext&) {
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
  void draw(SkCanvas* c, const SkMatrix&, CdlLiteDL::DrawContext&) {
    SkPaint pt = paint.toSkPaint();
    c->drawImageRect(image.get(), src, dst, &pt, constraint);
  }
};

struct DrawImageLattice final : Op {
  static const auto kType = Type::DrawImageLattice;
  DrawImageLattice(sk_sp<const SkImage>&& image,
                   int xs,
                   int ys,
                   int fs,
                   const SkIRect& src,
                   const SkRect& dst,
                   const SkPaint* paint)
      : image(std::move(image)), xs(xs), ys(ys), fs(fs), src(src), dst(dst) {
    if (paint) {
      this->paint = *paint;
    }
  }
  sk_sp<const SkImage> image;
  int xs, ys, fs;
  SkIRect src;
  SkRect dst;
  SkPaint paint;
  void draw(SkCanvas* c, const SkMatrix&, CdlLiteDL::DrawContext&) {
    auto xdivs = pod<int>(this, 0), ydivs = pod<int>(this, xs * sizeof(int));
    auto flags = (0 == fs) ? nullptr : pod<SkCanvas::Lattice::Flags>(
                                           this, (xs + ys) * sizeof(int));
    c->drawImageLattice(image.get(), {xdivs, ydivs, flags, xs, ys, &src}, dst,
                        &paint);
  }
};

struct DrawText final : Op {
  static const auto kType = Type::DrawText;
  DrawText(size_t bytes, SkScalar x, SkScalar y, const SkPaint& paint)
      : bytes(bytes), x(x), y(y), paint(paint) {}
  size_t bytes;
  SkScalar x, y;
  SkPaint paint;
  void draw(SkCanvas* c, const SkMatrix&, CdlLiteDL::DrawContext&) {
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
  void draw(SkCanvas* c, const SkMatrix&, CdlLiteDL::DrawContext&) {
    auto points = pod<SkPoint>(this);
    auto text = pod<void>(this, n * sizeof(SkPoint));
    c->drawPosText(text, bytes, points, paint);
  }
};
struct DrawPosTextH final : Op {
  static const auto kType = Type::DrawPosTextH;
  DrawPosTextH(size_t bytes, SkScalar y, const SkPaint& paint, int n)
      : bytes(bytes), y(y), paint(paint), n(n) {}
  size_t bytes;
  SkScalar y;
  SkPaint paint;
  int n;
  void draw(SkCanvas* c, const SkMatrix&, CdlLiteDL::DrawContext&) {
    auto xs = pod<SkScalar>(this);
    auto text = pod<void>(this, n * sizeof(SkScalar));
    c->drawPosTextH(text, bytes, xs, y, paint);
  }
};
struct DrawTextOnPath final : Op {
  static const auto kType = Type::DrawTextOnPath;
  DrawTextOnPath(size_t bytes,
                 const SkPath& path,
                 const SkMatrix* matrix,
                 const SkPaint& paint)
      : bytes(bytes), path(path), paint(paint) {
    if (matrix) {
      this->matrix = *matrix;
    }
  }
  size_t bytes;
  SkPath path;
  SkMatrix matrix = SkMatrix::I();
  SkPaint paint;
  void draw(SkCanvas* c, const SkMatrix&, CdlLiteDL::DrawContext&) {
    c->drawTextOnPath(pod<void>(this), bytes, path, &matrix, paint);
  }
  void makeThreadsafe() { make_threadsafe(&path, &matrix); }
};
struct DrawTextRSXform final : Op {
  static const auto kType = Type::DrawTextRSXform;
  DrawTextRSXform(size_t bytes, const SkRect* cull, const SkPaint& paint)
      : bytes(bytes), paint(paint) {
    if (cull) {
      this->cull = *cull;
    }
  }
  size_t bytes;
  SkRect cull = kUnset;
  SkPaint paint;
  void draw(SkCanvas* c, const SkMatrix&, CdlLiteDL::DrawContext&) {
    c->drawTextRSXform(pod<void>(this), bytes, pod<SkRSXform>(this, bytes),
                       maybe_unset(cull), paint);
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
  void draw(SkCanvas* c, const SkMatrix&, CdlLiteDL::DrawContext& dc) {
    c->drawTextBlob(blob.get(), x, y, paint);
  }
};

struct DrawPatch final : Op {
  static const auto kType = Type::DrawPatch;
  DrawPatch(const SkPoint cubics[12],
            const SkColor colors[4],
            const SkPoint texs[4],
            SkBlendMode bmode,
            const SkPaint& paint)
      : xfermode(bmode), paint(paint) {
    copy_v(this->cubics, cubics, 12);
    if (colors) {
      copy_v(this->colors, colors, 4);
      has_colors = true;
    }
    if (texs) {
      copy_v(this->texs, texs, 4);
      has_texs = true;
    }
  }
  SkPoint cubics[12];
  SkColor colors[4];
  SkPoint texs[4];
  SkBlendMode xfermode;
  SkPaint paint;
  bool has_colors = false;
  bool has_texs = false;
  void draw(SkCanvas* c, const SkMatrix&, CdlLiteDL::DrawContext&) {
    c->drawPatch(cubics, has_colors ? colors : nullptr,
                 has_texs ? texs : nullptr, xfermode, paint);
  }
};
struct DrawPoints final : Op {
  static const auto kType = Type::DrawPoints;
  DrawPoints(SkCanvas::PointMode mode, size_t count, const SkPaint& paint)
      : mode(mode), count(count), paint(paint) {}
  SkCanvas::PointMode mode;
  size_t count;
  SkPaint paint;
  void draw(SkCanvas* c, const SkMatrix&, CdlLiteDL::DrawContext&) {
    c->drawPoints(mode, count, pod<SkPoint>(this), paint);
  }
};
struct DrawVertices final : Op {
  static const auto kType = Type::DrawVertices;
  DrawVertices(SkCanvas::VertexMode mode,
               int count,
               SkBlendMode bmode,
               int nindices,
               const SkPaint& paint,
               bool has_texs,
               bool has_colors,
               bool has_indices)
      : mode(mode),
        count(count),
        xfermode(bmode),
        nindices(nindices),
        paint(paint),
        has_texs(has_texs),
        has_colors(has_colors),
        has_indices(has_indices) {}
  SkCanvas::VertexMode mode;
  int count;
  SkBlendMode xfermode;
  int nindices;
  SkPaint paint;
  bool has_texs;
  bool has_colors;
  bool has_indices;
  void draw(SkCanvas* c, const SkMatrix&, CdlLiteDL::DrawContext&) {
    SkPoint* vertices = pod<SkPoint>(this, 0);
    size_t offset = count * sizeof(SkPoint);

    SkPoint* texs = nullptr;
    if (has_texs) {
      texs = pod<SkPoint>(this, offset);
      offset += count * sizeof(SkPoint);
    }

    SkColor* colors = nullptr;
    if (has_colors) {
      colors = pod<SkColor>(this, offset);
      offset += count * sizeof(SkColor);
    }

    uint16_t* indices = nullptr;
    if (has_indices) {
      indices = pod<uint16_t>(this, offset);
    }
    c->drawVertices(mode, count, vertices, texs, colors, xfermode, indices,
                    nindices, paint);
  }
};
struct DrawAtlas final : Op {
  static const auto kType = Type::DrawAtlas;
  DrawAtlas(const SkImage* atlas,
            int count,
            SkBlendMode xfermode,
            const SkRect* cull,
            const SkPaint* paint,
            bool has_colors)
      : atlas(sk_ref_sp(atlas)),
        count(count),
        xfermode(xfermode),
        has_colors(has_colors) {
    if (cull) {
      this->cull = *cull;
    }
    if (paint) {
      this->paint = *paint;
    }
  }
  sk_sp<const SkImage> atlas;
  int count;
  SkBlendMode xfermode;
  SkRect cull = kUnset;
  SkPaint paint;
  bool has_colors;
  void draw(SkCanvas* c, const SkMatrix&, CdlLiteDL::DrawContext&) {
    auto xforms = pod<SkRSXform>(this, 0);
    auto texs = pod<SkRect>(this, count * sizeof(SkRSXform));
    auto colors =
        has_colors
            ? pod<SkColor>(this, count * (sizeof(SkRSXform) + sizeof(SkRect)))
            : nullptr;
    c->drawAtlas(atlas.get(), xforms, texs, colors, count, xfermode,
                 maybe_unset(cull), &paint);
  }
};
}

template <typename T, typename... Args>
void* CdlLiteDL::push(size_t pod, Args&&... args) {
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
inline void CdlLiteDL::map(const Fn fns[], Args... args) {
  auto end = fBytes.get() + fUsed;
  for (uint8_t* ptr = fBytes.get(); ptr < end;) {
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
void CdlLiteDL::setDrawFilter(SkDrawFilter* df) {
  this->push<SetDrawFilter>(0, df);
}
#endif

void CdlLiteDL::save() {
  this->push<Save>(0);
}
void CdlLiteDL::restore() {
  this->push<Restore>(0);
}
void CdlLiteDL::saveLayer(const SkRect* bounds,
                          const SkPaint* paint,
                          const SkImageFilter* backdrop,
                          SkCanvas::SaveLayerFlags flags) {
  this->push<SaveLayer>(0, bounds, paint, backdrop, flags);
}

void CdlLiteDL::concat(const SkMatrix& matrix) {
  this->push<Concat>(0, matrix);
}
void CdlLiteDL::setMatrix(const SkMatrix& matrix) {
  this->push<SetMatrix>(0, matrix);
}
void CdlLiteDL::translate(SkScalar dx, SkScalar dy) {
  this->push<Translate>(0, dx, dy);
}
void CdlLiteDL::translateZ(SkScalar dz) {
  this->push<TranslateZ>(0, dz);
}

void CdlLiteDL::clipPath(const SkPath& path, SkCanvas::ClipOp op, bool aa) {
  this->push<ClipPath>(0, path, op, aa);
}
void CdlLiteDL::clipRect(const SkRect& rect, SkCanvas::ClipOp op, bool aa) {
  this->push<ClipRect>(0, rect, op, aa);
}
void CdlLiteDL::clipRRect(const SkRRect& rrect, SkCanvas::ClipOp op, bool aa) {
  this->push<ClipRRect>(0, rrect, op, aa);
}
void CdlLiteDL::clipRegion(const SkRegion& region, SkCanvas::ClipOp op) {
  this->push<ClipRegion>(0, region, op);
}

void CdlLiteDL::drawPaint(const SkPaint& paint) {
  this->push<DrawPaint>(0, paint);
}
void CdlLiteDL::drawPath(const SkPath& path, const SkPaint& paint) {
  this->push<DrawPath>(0, path, paint);
}
void CdlLiteDL::drawRect(const SkRect& rect, const SkPaint& paint) {
  this->push<DrawRect>(0, rect, paint);
}
void CdlLiteDL::drawRect(const SkRect& rect, const CdlPaint& paint) {
  this->push<DrawRectX>(0, rect, paint);
}

void CdlLiteDL::drawRegion(const SkRegion& region, const SkPaint& paint) {
  this->push<DrawRegion>(0, region, paint);
}
void CdlLiteDL::drawOval(const SkRect& oval, const SkPaint& paint) {
  this->push<DrawOval>(0, oval, paint);
}
void CdlLiteDL::drawArc(const SkRect& oval,
                        SkScalar startAngle,
                        SkScalar sweepAngle,
                        bool useCenter,
                        const SkPaint& paint) {
  this->push<DrawArc>(0, oval, startAngle, sweepAngle, useCenter, paint);
}
void CdlLiteDL::drawRRect(const SkRRect& rrect, const SkPaint& paint) {
  this->push<DrawRRect>(0, rrect, paint);
}
void CdlLiteDL::drawDRRect(const SkRRect& outer,
                           const SkRRect& inner,
                           const SkPaint& paint) {
  this->push<DrawDRRect>(0, outer, inner, paint);
}

void CdlLiteDL::drawAnnotation(const SkRect& rect,
                               const char* key,
                               SkData* value) {
  size_t bytes = strlen(key) + 1;
  void* pod = this->push<DrawAnnotation>(bytes, rect, value);
  copy_v(pod, key, bytes);
}
void CdlLiteDL::drawDrawable(SkDrawable* drawable, const SkMatrix* matrix) {
  this->push<DrawDrawable>(0, drawable, matrix);
}
void CdlLiteDL::drawPicture(const SkPicture* picture,
                            const SkMatrix* matrix,
                            const SkPaint* paint) {
  this->push<DrawPicture>(0, picture, matrix, paint);
}
void CdlLiteDL::drawShadowedPicture(const SkPicture* picture,
                                    const SkMatrix* matrix,
                                    const SkPaint* paint,
                                    const SkShadowParams& params) {
  push<DrawShadowedPicture>(0, picture, matrix, paint, params);
}

void CdlLiteDL::drawImage(sk_sp<const SkImage> image,
                          SkScalar x,
                          SkScalar y,
                          const SkPaint* paint) {
  this->push<DrawImage>(0, std::move(image), x, y, paint);
}
void CdlLiteDL::drawImage(sk_sp<const SkImage> image,
                          SkScalar x,
                          SkScalar y,
                          const CdlPaint& paint) {
  this->push<DrawImageX>(0, std::move(image), x, y, paint);
}

void CdlLiteDL::drawImageNine(sk_sp<const SkImage> image,
                              const SkIRect& center,
                              const SkRect& dst,
                              const SkPaint* paint) {
  this->push<DrawImageNine>(0, std::move(image), center, dst, paint);
}
void CdlLiteDL::drawImageRect(sk_sp<const SkImage> image,
                              const SkRect* src,
                              const SkRect& dst,
                              const SkPaint* paint,
                              SkCanvas::SrcRectConstraint constraint) {
  this->push<DrawImageRect>(0, std::move(image), src, dst, paint, constraint);
}
void CdlLiteDL::drawImageRect(sk_sp<const SkImage> image,
                              const SkRect* src,
                              const SkRect& dst,
                              const CdlPaint& paint,
                              SkCanvas::SrcRectConstraint constraint) {
  this->push<DrawImageRectX>(0, std::move(image), src, dst, paint, constraint);
}
void CdlLiteDL::drawImageLattice(sk_sp<const SkImage> image,
                                 const SkCanvas::Lattice& lattice,
                                 const SkRect& dst,
                                 const SkPaint* paint) {
  int xs = lattice.fXCount, ys = lattice.fYCount;
  int fs = lattice.fFlags ? (xs + 1) * (ys + 1) : 0;
  size_t bytes =
      (xs + ys) * sizeof(int) + fs * sizeof(SkCanvas::Lattice::Flags);
  SkASSERT(lattice.fBounds);
  void* pod = this->push<DrawImageLattice>(bytes, std::move(image), xs, ys, fs,
                                           *lattice.fBounds, dst, paint);
  copy_v(pod, lattice.fXDivs, xs, lattice.fYDivs, ys, lattice.fFlags, fs);
}

void CdlLiteDL::drawText(const void* text,
                         size_t bytes,
                         SkScalar x,
                         SkScalar y,
                         const SkPaint& paint) {
  void* pod = this->push<DrawText>(bytes, bytes, x, y, paint);
  copy_v(pod, (const char*)text, bytes);
}
void CdlLiteDL::drawPosText(const void* text,
                            size_t bytes,
                            const SkPoint pos[],
                            const SkPaint& paint) {
  int n = paint.countText(text, bytes);
  void* pod =
      this->push<DrawPosText>(n * sizeof(SkPoint) + bytes, bytes, paint, n);
  copy_v(pod, pos, n, (const char*)text, bytes);
}
void CdlLiteDL::drawPosTextH(const void* text,
                             size_t bytes,
                             const SkScalar xs[],
                             SkScalar y,
                             const SkPaint& paint) {
  int n = paint.countText(text, bytes);
  void* pod = this->push<DrawPosTextH>(n * sizeof(SkScalar) + bytes, bytes, y,
                                       paint, n);
  copy_v(pod, xs, n, (const char*)text, bytes);
}
void CdlLiteDL::drawTextOnPath(const void* text,
                               size_t bytes,
                               const SkPath& path,
                               const SkMatrix* matrix,
                               const SkPaint& paint) {
  void* pod = this->push<DrawTextOnPath>(bytes, bytes, path, matrix, paint);
  copy_v(pod, (const char*)text, bytes);
}
void CdlLiteDL::drawTextRSXform(const void* text,
                                size_t bytes,
                                const SkRSXform xforms[],
                                const SkRect* cull,
                                const SkPaint& paint) {
  int n = paint.countText(text, bytes);
  void* pod = this->push<DrawTextRSXform>(bytes + n * sizeof(SkRSXform), bytes,
                                          cull, paint);
  copy_v(pod, (const char*)text, bytes, xforms, n);
}
void CdlLiteDL::drawTextBlob(const SkTextBlob* blob,
                             SkScalar x,
                             SkScalar y,
                             const SkPaint& paint) {
  this->push<DrawTextBlob>(0, blob, x, y, paint);
}

void CdlLiteDL::drawPatch(const SkPoint points[12],
                          const SkColor colors[4],
                          const SkPoint texs[4],
                          SkBlendMode bmode,
                          const SkPaint& paint) {
  this->push<DrawPatch>(0, points, colors, texs, bmode, paint);
}
void CdlLiteDL::drawPoints(SkCanvas::PointMode mode,
                           size_t count,
                           const SkPoint points[],
                           const SkPaint& paint) {
  void* pod =
      this->push<DrawPoints>(count * sizeof(SkPoint), mode, count, paint);
  copy_v(pod, points, count);
}
void CdlLiteDL::drawVertices(SkCanvas::VertexMode mode,
                             int count,
                             const SkPoint vertices[],
                             const SkPoint texs[],
                             const SkColor colors[],
                             SkBlendMode xfermode,
                             const uint16_t indices[],
                             int nindices,
                             const SkPaint& paint) {
  size_t bytes = count * sizeof(SkPoint);
  if (texs) {
    bytes += count * sizeof(SkPoint);
  }
  if (colors) {
    bytes += count * sizeof(SkColor);
  }
  if (indices) {
    bytes += nindices * sizeof(uint16_t);
  }
  void* pod = this->push<DrawVertices>(bytes, mode, count, xfermode, nindices,
                                       paint, texs != nullptr,
                                       colors != nullptr, indices != nullptr);
  copy_v(pod, vertices, count, texs, texs ? count : 0, colors,
         colors ? count : 0, indices, indices ? nindices : 0);
}
void CdlLiteDL::drawAtlas(const SkImage* atlas,
                          const SkRSXform xforms[],
                          const SkRect texs[],
                          const SkColor colors[],
                          int count,
                          SkBlendMode xfermode,
                          const SkRect* cull,
                          const SkPaint* paint) {
  size_t bytes = count * (sizeof(SkRSXform) + sizeof(SkRect));
  if (colors) {
    bytes += count * sizeof(SkColor);
  }
  void* pod = this->push<DrawAtlas>(bytes, atlas, count, xfermode, cull, paint,
                                    colors != nullptr);
  copy_v(pod, xforms, count, texs, count, colors, colors ? count : 0);
}

typedef void (*draw_fn)(void*,
                        SkCanvas*,
                        const SkMatrix&,
                        CdlLiteDL::DrawContext&);
typedef void (*void_fn)(void*);

// All ops implement draw().
#define M(T)                                          \
  [](void* op, SkCanvas* c, const SkMatrix& original, \
     CdlLiteDL::DrawContext& dc) { ((T*)op)->draw(c, original, dc); },
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

void CdlLiteDL::onDraw(SkCanvas* canvas) {
  DrawContext dc;
  this->map(draw_fns, canvas, canvas->getTotalMatrix(), dc);
}
void CdlLiteDL::makeThreadsafe() {
  this->map(make_threadsafe_fns);
}

SkRect CdlLiteDL::onGetBounds() {
  return fBounds;
}

CdlLiteDL::CdlLiteDL(SkRect bounds) : fUsed(0), fReserved(0), fBounds(bounds) {}

CdlLiteDL::~CdlLiteDL() {
  this->reset(SkRect::MakeEmpty());
}

void CdlLiteDL::reset(SkRect bounds) {
  SkASSERT(this->unique());
  this->map(dtor_fns);

  // Leave fBytes and fReserved alone.
  fUsed = 0;
  fBounds = bounds;
}

void CdlLiteDL::drawAsLayer(SkCanvas* canvas,
                            const SkMatrix* matrix,
                            const SkPaint* paint) {
  auto fallback_plan = [&] {
    SkRect bounds = this->getBounds();
    canvas->saveLayer(&bounds, paint);
    this->draw(canvas, matrix);
    canvas->restore();
  };

  // TODO: single-draw specializations

  return fallback_plan();
}

void CdlLiteDL::setBounds(const SkRect& bounds) {
  fBounds = bounds;
}
