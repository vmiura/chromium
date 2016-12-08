/*
 * Copyright 2016 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "cdl_shader.h"

#include "base/synchronization/lock.h"
#include "skia/ext/cdl_picture.h"
#include "third_party/skia/include/core/SkMatrix.h"
#include "third_party/skia/include/core/SkPicture.h"
#include "third_party/skia/include/core/SkShader.h"

CdlShader::CdlShader(const SkMatrix* local_matrix) {
  if (local_matrix) {
    local_matrix_ = *local_matrix;
  } else {
    local_matrix_.reset();
  }
  // Pre-cache so future calls to local_matrix_.getType() are threadsafe.
  (void)local_matrix_.getType();
}

CdlShader::~CdlShader() {}

bool CdlShader::isOpaque() const {
  return false;
}

////////////////////////////////////////////////////////////////////////////////

class CdlWrapSkShader : public CdlShader {
 public:
  CdlWrapSkShader(sk_sp<SkShader> shader) : shader_(std::move(shader)) {}

  sk_sp<SkShader> createSkShader() override {
    LOG(ERROR) << "createSkShader" << shader_.get();
    return shader_;
  }

  bool isOpaque() const override { return shader_->isOpaque(); }

 private:
  sk_sp<SkShader> shader_;
};

sk_sp<CdlShader> CdlShader::WrapSkShader(sk_sp<SkShader> shader) {
  return sk_make_sp<CdlWrapSkShader>(std::move(shader));
}

////////////////////////////////////////////////////////////////////////////////

class CdlImageShader : public CdlShader {
 public:
  CdlImageShader(sk_sp<SkImage> image,
                 SkShader::TileMode tmx,
                 SkShader::TileMode tmy,
                 const SkMatrix* local_matrix)
      : CdlShader(local_matrix),
        image_(std::move(image)),
        tmx_(tmx),
        tmy_(tmy) {}

  sk_sp<SkShader> createSkShader() override {
    base::AutoLock hold(shader_lock_);
    if (shader_.get())
      return shader_;

    return shader_ = image_->makeShader(tmx_, tmy_, &getLocalMatrix());
  }

  bool isOpaque() const override { return image_->isOpaque(); }

 private:
  sk_sp<SkImage> image_;
  SkShader::TileMode tmx_, tmy_;

  base::Lock shader_lock_;
  sk_sp<SkShader> shader_;
};

sk_sp<CdlShader> CdlShader::MakeImageShader(sk_sp<SkImage> image,
                                            SkShader::TileMode tmx,
                                            SkShader::TileMode tmy,
                                            const SkMatrix* local_matrix) {
  return sk_sp<CdlShader>(new CdlImageShader(image, tmx, tmy, local_matrix));
}

////////////////////////////////////////////////////////////////////////////////

class CdlPictureShader : public CdlShader {
 public:
  CdlPictureShader(sk_sp<CdlPicture> picture,
                   SkShader::TileMode tmx,
                   SkShader::TileMode tmy,
                   const SkMatrix* local_matrix,
                   const SkRect* tile)
      : CdlShader(local_matrix),
        picture_(std::move(picture)),
        tile_(tile ? *tile : picture_->cullRect()),
        tmx_(tmx),
        tmy_(tmy) {}

  sk_sp<SkShader> createSkShader() override {
    base::AutoLock hold(shader_lock_);
    if (shader_.get())
      return shader_;

    return shader_ = SkShader::MakePictureShader(
               picture_->toSkPicture(), tmx_, tmy_, &getLocalMatrix(), &tile_);
  }

 private:
  sk_sp<CdlPicture> picture_;
  SkRect tile_;
  SkShader::TileMode tmx_, tmy_;

  base::Lock shader_lock_;
  sk_sp<SkShader> shader_;
};

sk_sp<CdlShader> CdlShader::MakePictureShader(sk_sp<CdlPicture> picture,
                                              SkShader::TileMode tmx,
                                              SkShader::TileMode tmy,
                                              const SkMatrix* local_matrix,
                                              const SkRect* tile) {
  return sk_sp<CdlShader>(
      new CdlPictureShader(picture, tmx, tmy, local_matrix, tile));
}
