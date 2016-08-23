// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/resources/ui_resource_request.h"

#include "base/memory/ptr_util.h"
#include "cc/ipc/ui_resource.mojom.h"

namespace cc {

UIResourceRequest::UIResourceRequest(UIResourceRequestType type,
                                     UIResourceId id)
    : type_(type), id_(id) {
  DCHECK(type == UI_RESOURCE_DELETE);
}

UIResourceRequest::UIResourceRequest(UIResourceRequestType type,
                                     UIResourceId id,
                                     const UIResourceBitmap& bitmap)
    : type_(type), id_(id), bitmap_(new UIResourceBitmap(bitmap)) {}

UIResourceRequest::UIResourceRequest(const UIResourceRequest& request) {
  (*this) = request;
}

UIResourceRequest& UIResourceRequest::operator=(
    const UIResourceRequest& request) {
  type_ = request.type_;
  id_ = request.id_;
  if (request.bitmap_) {
    bitmap_ = base::WrapUnique(new UIResourceBitmap(*request.bitmap_.get()));
  } else {
    bitmap_ = nullptr;
  }

  return *this;
}

UIResourceRequest::~UIResourceRequest() {}

void UIResourceRequest::WriteMojom(
    cc::mojom::UIResourceRequestProperties* mojom) {
  mojom->uid = id_;
  switch (type_) {
    case UIResourceRequestType::UI_RESOURCE_CREATE:
      mojom->type = cc::mojom::UIResourceRequestType::CREATE_REQUEST;
      break;
    case UIResourceRequestType::UI_RESOURCE_DELETE:
      mojom->type = cc::mojom::UIResourceRequestType::DELETE;
      break;
    case UIResourceRequestType::UI_RESOURCE_INVALID_REQUEST:
      mojom->type = cc::mojom::UIResourceRequestType::INVALID;
      break;
  }
  if (bitmap_) {
    mojom->bitmap = mojom::UIResourceBitmap::New();
    bitmap_->WriteMojom(mojom->bitmap.get());
  }
}

}  // namespace cc
