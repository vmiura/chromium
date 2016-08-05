// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_IPC_TRANSFORM_OPERATION_STRUCT_TRAITS_H_
#define CC_IPC_TRANSFORM_OPERATION_STRUCT_TRAITS_H_

#include "cc/ipc/animation.mojom.h"
#include "cc/animation/transform_operation.h"

namespace mojo {

namespace {
cc::mojom::TransformOperationType CCTransformOperationTypeToMojo(
    const cc::TransformOperation::Type& type) {
  switch (type) {
    case cc::TransformOperation::TRANSFORM_OPERATION_TRANSLATE:
      return cc::mojom::TransformOperationType::TRANSFORM_OPERATION_TRANSLATE;
    case cc::TransformOperation::TRANSFORM_OPERATION_ROTATE:
      return cc::mojom::TransformOperationType::TRANSFORM_OPERATION_ROTATE;
    case cc::TransformOperation::TRANSFORM_OPERATION_SCALE:
      return cc::mojom::TransformOperationType::TRANSFORM_OPERATION_SCALE;
    case cc::TransformOperation::TRANSFORM_OPERATION_SKEW:
      return cc::mojom::TransformOperationType::TRANSFORM_OPERATION_SKEW;
    case cc::TransformOperation::TRANSFORM_OPERATION_PERSPECTIVE:
      return cc::mojom::TransformOperationType::TRANSFORM_OPERATION_PERSPECTIVE;
    case cc::TransformOperation::TRANSFORM_OPERATION_MATRIX:
      return cc::mojom::TransformOperationType::TRANSFORM_OPERATION_MATRIX;
    case cc::TransformOperation::TRANSFORM_OPERATION_IDENTITY:
      return cc::mojom::TransformOperationType::TRANSFORM_OPERATION_IDENTITY;
  };
  NOTREACHED();
  return cc::mojom::TransformOperationType::TRANSFORM_OPERATION_IDENTITY;
}

cc::TransformOperation::Type MojoTransformOperationTypeToCC(
    const cc::mojom::TransformOperationType& type) {
  switch (type) {
    case cc::mojom::TransformOperationType::TRANSFORM_OPERATION_TRANSLATE:
      return cc::TransformOperation::TRANSFORM_OPERATION_TRANSLATE;
    case cc::mojom::TransformOperationType::TRANSFORM_OPERATION_ROTATE:
      return cc::TransformOperation::TRANSFORM_OPERATION_ROTATE;
    case cc::mojom::TransformOperationType::TRANSFORM_OPERATION_SCALE:
      return cc::TransformOperation::TRANSFORM_OPERATION_SCALE;
    case cc::mojom::TransformOperationType::TRANSFORM_OPERATION_SKEW:
      return cc::TransformOperation::TRANSFORM_OPERATION_SKEW;
    case cc::mojom::TransformOperationType::TRANSFORM_OPERATION_PERSPECTIVE:
      return cc::TransformOperation::TRANSFORM_OPERATION_PERSPECTIVE;
    case cc::mojom::TransformOperationType::TRANSFORM_OPERATION_MATRIX:
      return cc::TransformOperation::TRANSFORM_OPERATION_MATRIX;
    case cc::mojom::TransformOperationType::TRANSFORM_OPERATION_IDENTITY:
      return cc::TransformOperation::TRANSFORM_OPERATION_IDENTITY;
  };
  NOTREACHED();
  return cc::TransformOperation::TRANSFORM_OPERATION_IDENTITY;
}

}  // namespace

template <>
struct StructTraits<cc::mojom::TransformOperation, cc::TransformOperation> {
  static const gfx::Transform& matrix(const cc::TransformOperation& operation) {
    return operation.matrix;
  }

  static float x(const cc::TransformOperation& operation) {
    switch (operation.type) {
      case cc::TransformOperation::TRANSFORM_OPERATION_TRANSLATE:
        return operation.translate.x;
      case cc::TransformOperation::TRANSFORM_OPERATION_ROTATE:
        return operation.rotate.axis.x;
      case cc::TransformOperation::TRANSFORM_OPERATION_SCALE:
        return operation.scale.x;
      case cc::TransformOperation::TRANSFORM_OPERATION_SKEW:
        return operation.skew.x;
      case cc::TransformOperation::TRANSFORM_OPERATION_PERSPECTIVE:
        return operation.perspective_depth;
      case cc::TransformOperation::TRANSFORM_OPERATION_MATRIX:
        NOTREACHED();
        return 0;
      case cc::TransformOperation::TRANSFORM_OPERATION_IDENTITY:
        return 0;
    };

    NOTREACHED();
    return 0;
  }

  static float y(const cc::TransformOperation& operation) {
    switch (operation.type) {
      case cc::TransformOperation::TRANSFORM_OPERATION_TRANSLATE:
        return operation.translate.y;
      case cc::TransformOperation::TRANSFORM_OPERATION_ROTATE:
        return operation.rotate.axis.y;
      case cc::TransformOperation::TRANSFORM_OPERATION_SCALE:
        return operation.scale.y;
      case cc::TransformOperation::TRANSFORM_OPERATION_SKEW:
        return operation.skew.y;
      case cc::TransformOperation::TRANSFORM_OPERATION_PERSPECTIVE:
        return operation.perspective_depth;
      case cc::TransformOperation::TRANSFORM_OPERATION_MATRIX:
        NOTREACHED();
        return 0;
      case cc::TransformOperation::TRANSFORM_OPERATION_IDENTITY:
        return 0;
    };

    NOTREACHED();
    return 0;
  }

  static float z(const cc::TransformOperation& operation) {
    switch (operation.type) {
      case cc::TransformOperation::TRANSFORM_OPERATION_TRANSLATE:
        return operation.translate.z;
      case cc::TransformOperation::TRANSFORM_OPERATION_ROTATE:
        return operation.rotate.axis.z;
      case cc::TransformOperation::TRANSFORM_OPERATION_SCALE:
        return operation.scale.z;
      case cc::TransformOperation::TRANSFORM_OPERATION_SKEW:
        NOTREACHED();
        return 0;
      case cc::TransformOperation::TRANSFORM_OPERATION_PERSPECTIVE:
        return operation.perspective_depth;
      case cc::TransformOperation::TRANSFORM_OPERATION_MATRIX:
        NOTREACHED();
        return 0;
      case cc::TransformOperation::TRANSFORM_OPERATION_IDENTITY:
        return 0;
    };

    NOTREACHED();
    return 0;
  }

  static float angle(const cc::TransformOperation& operation) {
    DCHECK_EQ(cc::TransformOperation::TRANSFORM_OPERATION_ROTATE, operation.type);
    return operation.rotate.angle;
  }

  static cc::mojom::TransformOperationType type(
      const cc::TransformOperation& operation) {
    return CCTransformOperationTypeToMojo(operation.type);
  }

  static bool Read(cc::mojom::TransformOperationDataView data,
                   cc::TransformOperation* out) {

    // TODO(hackathon): this should use enum traits.
    cc::mojom::TransformOperationType transform_operation_type =
        cc::mojom::TransformOperationType::TRANSFORM_OPERATION_IDENTITY;
    if (!data.ReadType(&transform_operation_type))
      return false;

    out->type = MojoTransformOperationTypeToCC(transform_operation_type);

    switch (out->type) {
      case cc::TransformOperation::TRANSFORM_OPERATION_TRANSLATE:
        out->translate.x = data.x();
        out->translate.y = data.y();
        out->translate.z = data.z();
        break;

      case cc::TransformOperation::TRANSFORM_OPERATION_ROTATE:
        out->rotate.axis.x = data.x();
        out->rotate.axis.y = data.y();
        out->rotate.axis.z = data.z();
        out->rotate.angle = data.angle();
        break;

      case cc::TransformOperation::TRANSFORM_OPERATION_SCALE:
        out->scale.x = data.x();
        out->scale.y = data.y();
        out->scale.z = data.z();
        break;

      case cc::TransformOperation::TRANSFORM_OPERATION_SKEW:
        out->skew.x = data.x();
        out->skew.y = data.y();
        break;

      case cc::TransformOperation::TRANSFORM_OPERATION_PERSPECTIVE:
        out->perspective_depth = data.x();
        break;

      case cc::TransformOperation::TRANSFORM_OPERATION_MATRIX:
        if (!data.ReadMatrix(&out->matrix))
          return false;
        break;

      case cc::TransformOperation::TRANSFORM_OPERATION_IDENTITY:
        break;
    };

    return true;
  }
};

}  // namespace mojo

#endif  // CC_IPC_TRANSFORM_OPERATION_STRUCT_TRAITS_H_
