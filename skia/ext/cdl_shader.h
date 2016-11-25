/*
 * Copyright 2016 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef SKIA_EXT_CDL_SHADER_H_
#define SKIA_EXT_CDL_SHADER_H_

#include "third_party/skia/include/core/SkMatrix.h"
#include "third_party/skia/include/core/SkRefCnt.h"
#include "third_party/skia/include/core/SkShader.h"

class CdlPicture;

class CdlShader : public SkRefCnt {
 public:
  CdlShader(const SkMatrix* localMatrix = NULL);
  ~CdlShader() override;

  static sk_sp<SkShader> MakeColorShader(SkColor);

  static sk_sp<CdlShader> MakeImageShader(sk_sp<SkImage>,
                                          SkShader::TileMode tx,
                                          SkShader::TileMode ty,
                                          const SkMatrix* localMatrix);

  static sk_sp<CdlShader> MakePictureShader(sk_sp<CdlPicture> picture,
                                            SkShader::TileMode tmx,
                                            SkShader::TileMode tmy,
                                            const SkMatrix* localMatrix,
                                            const SkRect* tile);

  const SkMatrix& getLocalMatrix() const { return fLocalMatrix; }

  virtual sk_sp<SkShader> createSkShader() = 0;

 private:
  SkMatrix fLocalMatrix;
};

#endif  // SKIA_EXT_CDL_SHADER_H_
