/*
 * Copyright 2016 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef SKIA_EXT_CDL_SHADER_H_
#define SKIA_EXT_CDL_SHADER_H_

#include "cdl_common.h"

#if CDL_ENABLED

#include "third_party/skia/include/core/SkMatrix.h"
#include "third_party/skia/include/core/SkRefCnt.h"
#include "third_party/skia/include/core/SkShader.h"

class CdlPicture;

class SK_API CdlShader : public SkRefCnt {
 public:
  CdlShader(const SkMatrix* local_matrix = NULL);
  ~CdlShader() override;

  static sk_sp<CdlShader> WrapSkShader(sk_sp<SkShader> shader);

  static sk_sp<CdlShader> MakeImageShader(sk_sp<SkImage>,
                                          SkShader::TileMode tx,
                                          SkShader::TileMode ty,
                                          const SkMatrix* local_matrix);

  static sk_sp<CdlShader> MakePictureShader(sk_sp<CdlPicture> picture,
                                            SkShader::TileMode tmx,
                                            SkShader::TileMode tmy,
                                            const SkMatrix* local_matrix,
                                            const SkRect* tile);

  const SkMatrix& getLocalMatrix() const { return local_matrix_; }

  virtual sk_sp<SkShader> createSkShader() = 0;
  virtual bool isOpaque() const;

 private:
  SkMatrix local_matrix_;
};

inline SK_API sk_sp<CdlShader> WrapSkShader(sk_sp<SkShader> shader) {
  return CdlShader::WrapSkShader(shader);
}

inline SK_API sk_sp<CdlShader> MakeCdlImageShader(sk_sp<SkImage> image,
                                           SkShader::TileMode tx,
                                           SkShader::TileMode ty,
                                           const SkMatrix* local_matrix) {
  return CdlShader::MakeImageShader(image, tx, ty, local_matrix);
}

#else  // CDL_ENABLED

#include "third_party/skia/include/core/SkImage.h"
#include "third_party/skia/include/core/SkShader.h"

inline SK_API sk_sp<CdlShader> WrapSkShader(sk_sp<SkShader> shader) {
  return shader;
}

inline SK_API sk_sp<CdlShader> MakeCdlImageShader(sk_sp<SkImage> image,
                                           SkShader::TileMode tx,
                                           SkShader::TileMode ty,
                                           const SkMatrix* local_matrix) {
  return image->makeShader(tx, ty, local_matrix);
}

#endif  // CDL_ENABLED

#endif  // SKIA_EXT_CDL_SHADER_H_
