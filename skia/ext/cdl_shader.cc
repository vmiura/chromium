/*
 * Copyright 2016 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "cdl_shader.h"

#include "base/debug/stack_trace.h"
#include "cdl_picture.h"
#include "third_party/skia/include/core/SkMatrix.h"
#include "third_party/skia/include/core/SkPicture.h"
#include "third_party/skia/include/core/SkShader.h"

CdlShader::CdlShader(const SkMatrix* localMatrix) {
  if (localMatrix) {
    fLocalMatrix = *localMatrix;
  } else {
    fLocalMatrix.reset();
  }
  // Pre-cache so future calls to fLocalMatrix.getType() are threadsafe.
  (void)fLocalMatrix.getType();
}

CdlShader::~CdlShader() {}

////////////////////////////////////////////////////////////////////////////////

class CdlWrapSkShader : public CdlShader {
 public:
  CdlWrapSkShader(sk_sp<SkShader> shader) : fShader(shader) {}

  sk_sp<SkShader> createSkShader() override { return fShader; }

 private:
  sk_sp<SkShader> fShader;
};

sk_sp<CdlShader> CdlShader::WrapSkShader(sk_sp<SkShader> shader) {
  return sk_sp<CdlShader>(new CdlWrapSkShader(shader));
}

////////////////////////////////////////////////////////////////////////////////

class CdlImageShader : public CdlShader {
 public:
  CdlImageShader(sk_sp<SkImage> image,
                 SkShader::TileMode tmx,
                 SkShader::TileMode tmy,
                 const SkMatrix* localMatrix)
      : CdlShader(localMatrix),
        fImage(std::move(image)),
        fTmx(tmx),
        fTmy(tmy) {}

  sk_sp<SkShader> createSkShader() override {
    if (fShader.get())
      return fShader;

    return fShader = fImage->makeShader(fTmx, fTmy, &getLocalMatrix());
  }

 private:
  sk_sp<SkImage> fImage;
  SkShader::TileMode fTmx, fTmy;
  sk_sp<SkShader> fShader;
};

sk_sp<CdlShader> CdlShader::MakeImageShader(sk_sp<SkImage> image,
                                            SkShader::TileMode tmx,
                                            SkShader::TileMode tmy,
                                            const SkMatrix* localMatrix) {
  return sk_sp<CdlShader>(new CdlImageShader(image, tmx, tmy, localMatrix));
}

////////////////////////////////////////////////////////////////////////////////

class CdlPictureShader : public CdlShader {
 public:
  CdlPictureShader(sk_sp<CdlPicture> picture,
                   SkShader::TileMode tmx,
                   SkShader::TileMode tmy,
                   const SkMatrix* localMatrix,
                   const SkRect* tile)
      : CdlShader(localMatrix),
        fPicture(std::move(picture)),
        fTile(tile ? *tile : fPicture->cullRect()),
        fTmx(tmx),
        fTmy(tmy) {}

  sk_sp<SkShader> createSkShader() override {
    if (fShader.get())
      return fShader;

    return fShader = SkShader::MakePictureShader(
               fPicture->toSkPicture(), fTmx, fTmy, &getLocalMatrix(), &fTile);
  }

 private:
  sk_sp<CdlPicture> fPicture;
  SkRect fTile;
  SkShader::TileMode fTmx, fTmy;
  sk_sp<SkShader> fShader;
};

sk_sp<CdlShader> CdlShader::MakePictureShader(sk_sp<CdlPicture> picture,
                                              SkShader::TileMode tmx,
                                              SkShader::TileMode tmy,
                                              const SkMatrix* localMatrix,
                                              const SkRect* tile) {
  return sk_sp<CdlShader>(
      new CdlPictureShader(picture, tmx, tmy, localMatrix, tile));
}
