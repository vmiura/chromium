// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/playback/transform_display_item.h"

#include <stddef.h>

#include "base/strings/stringprintf.h"
#include "base/trace_event/trace_event_argument.h"
#include "cc/playback/display_item_list.h"
#include "cc/proto/display_item.pb.h"
#include "cc/proto/gfx_conversions.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkStream.h"

namespace cc {

TransformDisplayItem::TransformDisplayItem(const gfx::Transform& transform)
    : transform_(gfx::Transform::kSkipInitialization) {
  SetNew(transform);
}

TransformDisplayItem::TransformDisplayItem(const proto::DisplayItem& proto) {
  DCHECK_EQ(proto::DisplayItem::Type_Transform, proto.type());

  const proto::TransformDisplayItem& details = proto.transform_item();
  gfx::Transform transform = ProtoToTransform(details.transform());

  SetNew(transform);
}

TransformDisplayItem::~TransformDisplayItem() {
}

void TransformDisplayItem::SetNew(const gfx::Transform& transform) {
  transform_ = transform;
}

void TransformDisplayItem::ToProtobuf(proto::DisplayItem* proto) const {
  proto->set_type(proto::DisplayItem::Type_Transform);

  proto::TransformDisplayItem* details = proto->mutable_transform_item();
  TransformToProto(transform_, details->mutable_transform());
}

void TransformDisplayItem::Serialize(SkWStream* stream, bool flush_cache) const {
  stream->write32(Transform);
  for (int row = 0; row < 4; ++row) {
    for (int col = 0; col < 4; ++col) {
      SkMScalar value = transform_.matrix().get(row, col);
      stream->write(&value, sizeof(SkMScalar));
    }
  }
}

void TransformDisplayItem::Deserialize(SkStream* stream,
                                         DisplayItemList* list,
                                         const gfx::Rect& visual_rect) {
  gfx::Transform transform;
  for (int row = 0; row < 4; ++row) {
    for (int col = 0; col < 4; ++col) {
      SkMScalar value;
      stream->read(&value, sizeof(SkMScalar));
      transform.matrix().set(row, col, value);
    }
  }
  list->CreateAndAppendItem<TransformDisplayItem>(visual_rect, transform);
}



void TransformDisplayItem::Raster(SkCanvas* canvas,
                                  SkPicture::AbortCallback* callback) const {
  canvas->save();
  if (!transform_.IsIdentity())
    canvas->concat(transform_.matrix());
}

void TransformDisplayItem::AsValueInto(
    const gfx::Rect& visual_rect,
    base::trace_event::TracedValue* array) const {
  array->AppendString(base::StringPrintf(
      "TransformDisplayItem transform: [%s] visualRect: [%s]",
      transform_.ToString().c_str(), visual_rect.ToString().c_str()));
}

size_t TransformDisplayItem::ExternalMemoryUsage() const {
  return 0;
}

EndTransformDisplayItem::EndTransformDisplayItem() {}

EndTransformDisplayItem::EndTransformDisplayItem(
    const proto::DisplayItem& proto) {
  DCHECK_EQ(proto::DisplayItem::Type_EndTransform, proto.type());
}

EndTransformDisplayItem::~EndTransformDisplayItem() {
}

void EndTransformDisplayItem::ToProtobuf(proto::DisplayItem* proto) const {
  proto->set_type(proto::DisplayItem::Type_EndTransform);
}

void EndTransformDisplayItem::Raster(
    SkCanvas* canvas,
    SkPicture::AbortCallback* callback) const {
  canvas->restore();
}

void EndTransformDisplayItem::AsValueInto(
    const gfx::Rect& visual_rect,
    base::trace_event::TracedValue* array) const {
  array->AppendString(
      base::StringPrintf("EndTransformDisplayItem visualRect: [%s]",
                         visual_rect.ToString().c_str()));
}

void EndTransformDisplayItem::Serialize(SkWStream* stream, bool flush_cache) const {
  stream->write32(EndTransform);
}

void EndTransformDisplayItem::Deserialize(SkStream* stream,
                                         DisplayItemList* list,
                                         const gfx::Rect& visual_rect) {
  list->CreateAndAppendItem<EndTransformDisplayItem>(visual_rect);
}




size_t EndTransformDisplayItem::ExternalMemoryUsage() const {
  return 0;
}

}  // namespace cc
