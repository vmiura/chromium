// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/playback/display_item_list.h"

#include <stddef.h>

#include <string>

#include "base/numerics/safe_conversions.h"
#include "base/strings/stringprintf.h"
#include "base/trace_event/trace_event.h"
#include "base/trace_event/trace_event_argument.h"
#include "cc/base/math_util.h"
#include "cc/debug/picture_debug_util.h"
#include "cc/debug/traced_display_item_list.h"
#include "cc/debug/traced_value.h"
#include "cc/playback/clip_display_item.h"
#include "cc/playback/clip_path_display_item.h"
#include "cc/playback/compositing_display_item.h"
#include "cc/playback/display_item_list_settings.h"
#include "cc/playback/display_item_proto_factory.h"
#include "cc/playback/drawing_display_item.h"
#include "cc/playback/filter_display_item.h"
#include "cc/playback/float_clip_display_item.h"
#include "cc/playback/largest_display_item.h"
#include "cc/playback/transform_display_item.h"
#include "cc/proto/display_item.pb.h"
#include "cc/proto/gfx_conversions.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkPictureRecorder.h"
#include "third_party/skia/include/core/SkStream.h"
#include "third_party/skia/include/utils/SkPictureUtils.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/skia_util.h"

namespace cc {

uint32_t DisplayItemList::s_next_id = 1;

namespace {


// We don't perform per-layer solid color analysis when there are too many skia
// operations.
const int kOpCountThatIsOkToAnalyze = 10;

bool DisplayItemsTracingEnabled() {
  bool tracing_enabled;
  TRACE_EVENT_CATEGORY_GROUP_ENABLED(
      TRACE_DISABLED_BY_DEFAULT("cc.debug.display_items"), &tracing_enabled);
  return tracing_enabled;
}

const int kDefaultNumDisplayItemsToReserve = 100;

}  // namespace

DisplayItemList::Inputs::Inputs(gfx::Rect layer_rect,
                                const DisplayItemListSettings& settings)
    : items(LargestDisplayItemSize(),
            LargestDisplayItemSize() * kDefaultNumDisplayItemsToReserve),
      settings(settings),
      layer_rect(layer_rect),
      is_suitable_for_gpu_rasterization(true) {}

DisplayItemList::Inputs::~Inputs() {}

scoped_refptr<DisplayItemList> DisplayItemList::Create(
    const gfx::Rect& layer_rect,
    const DisplayItemListSettings& settings) {
  return make_scoped_refptr(new DisplayItemList(
      layer_rect, settings,
      !settings.use_cached_picture || DisplayItemsTracingEnabled()));
}

scoped_refptr<DisplayItemList> DisplayItemList::CreateFromData(sk_sp<SkData> data) {
  // TODO: not used, delete this.
  SkMemoryStream read_stream(data);
  scoped_refptr<DisplayItemList> output_list;
  return output_list;
}

scoped_refptr<DisplayItemList> DisplayItemList::CreateFromStream(
    SkStream* stream,
    PictureCache* old_picture_cache,
    PictureCache* new_picture_cache) {
  DisplayItemListSettings settings;
  // TODO: Also find id from stream.
  scoped_refptr<DisplayItemList> display_item_list =
      DisplayItemList::Create(gfx::Rect(), settings);
  display_item_list->id_ = stream->readU32();
  uint32_t num_items = stream->readU32();
  for (uint32_t i = 0; i < num_items; ++i) {
    int x = stream->readS32();
    int y = stream->readS32();
    int width = stream->readS32();
    int height = stream->readS32();
    gfx::Rect visual_rect(x, y, width, height);
    DisplayItem::ItemType item_type =
        static_cast<DisplayItem::ItemType>(stream->readU32());
    switch(item_type) {
      case DisplayItem::Clip:
        ClipDisplayItem::Deserialize(stream, display_item_list.get(), visual_rect);
        break;
      case DisplayItem::EndClip:
        EndClipDisplayItem::Deserialize(stream, display_item_list.get(), visual_rect);
        break;
      case DisplayItem::ClipPath:
        ClipPathDisplayItem::Deserialize(stream, display_item_list.get(), visual_rect);
        break;
      case DisplayItem::EndClipPath:
        EndClipPathDisplayItem::Deserialize(stream, display_item_list.get(), visual_rect);
        break;
      case DisplayItem::Compositing:
        CompositingDisplayItem::Deserialize(stream, display_item_list.get(), visual_rect);
        break;
      case DisplayItem::EndCompositing:
        EndCompositingDisplayItem::Deserialize(stream, display_item_list.get(), visual_rect);
        break;
      case DisplayItem::Drawing: {
        uint32_t picture_id = stream->readU32();
        auto it = new_picture_cache->find(picture_id);
        if (it == new_picture_cache->end()) {
          sk_sp<const SkPicture> picture;
          it = old_picture_cache->find(picture_id);
          if (it == old_picture_cache->end()) {
            DrawingDisplayItem::Deserialize(picture_id, stream, display_item_list.get(), visual_rect);
            picture = display_item_list->inputs_.items.last().GetPicture();
          } else {
            picture = std::move(it->second);
            display_item_list->CreateAndAppendItem<DrawingDisplayItem>(
                visual_rect, picture);
          }
          (*new_picture_cache)[picture_id] = std::move(picture);
        } else {
          display_item_list->CreateAndAppendItem<DrawingDisplayItem>(
              visual_rect, it->second);
        }
        break;
      }
      case DisplayItem::Filter:
        FilterDisplayItem::Deserialize(stream, display_item_list.get(), visual_rect);
        break;
      case DisplayItem::EndFilter:
        EndFilterDisplayItem::Deserialize(stream, display_item_list.get(), visual_rect);
        break;
      case DisplayItem::FloatClip:
        FloatClipDisplayItem::Deserialize(stream, display_item_list.get(), visual_rect);
        break;
      case DisplayItem::EndFloatClip:
        EndFloatClipDisplayItem::Deserialize(stream, display_item_list.get(), visual_rect);
        break;
      case DisplayItem::Transform:
        TransformDisplayItem::Deserialize(stream, display_item_list.get(), visual_rect);
        break;
      case DisplayItem::EndTransform:
        EndTransformDisplayItem::Deserialize(stream, display_item_list.get(), visual_rect);
        break;
    }
  }
  display_item_list->Finalize();
  return display_item_list;
}

scoped_refptr<DisplayItemList> DisplayItemList::CreateFromProto(
    const proto::DisplayItemList& proto,
    ClientPictureCache* client_picture_cache,
    std::vector<uint32_t>* used_engine_picture_ids) {
  gfx::Rect layer_rect = ProtoToRect(proto.layer_rect());
  scoped_refptr<DisplayItemList> list =
      DisplayItemList::Create(ProtoToRect(proto.layer_rect()),
                              DisplayItemListSettings(proto.settings()));

  for (int i = 0; i < proto.items_size(); i++) {
    const proto::DisplayItem& item_proto = proto.items(i);
    DisplayItemProtoFactory::AllocateAndConstruct(
        layer_rect, list.get(), item_proto, client_picture_cache,
        used_engine_picture_ids);
  }

  list->Finalize();

  return list;
}

DisplayItemList::DisplayItemList(gfx::Rect layer_rect,
                                 const DisplayItemListSettings& settings,
                                 bool retain_individual_display_items)
    : retain_individual_display_items_(retain_individual_display_items),
      approximate_op_count_(0),
      picture_memory_usage_(0),
      inputs_(layer_rect, settings),
      id_(s_next_id++) {
  if (inputs_.settings.use_cached_picture) {
    SkRTreeFactory factory;
    recorder_.reset(new SkPictureRecorder());

    SkCanvas* canvas = recorder_->beginRecording(
        inputs_.layer_rect.width(), inputs_.layer_rect.height(), &factory);
    canvas->translate(-inputs_.layer_rect.x(), -inputs_.layer_rect.y());
    canvas->clipRect(gfx::RectToSkRect(inputs_.layer_rect));
  }
}

DisplayItemList::~DisplayItemList() {
}

void DisplayItemList::ToProtobuf(proto::DisplayItemList* proto) {
  // The flattened SkPicture approach is going away, and the proto
  // doesn't currently support serializing that flattened picture.
  DCHECK(retain_individual_display_items_);

  RectToProto(inputs_.layer_rect, proto->mutable_layer_rect());
  inputs_.settings.ToProtobuf(proto->mutable_settings());

  DCHECK_EQ(0, proto->items_size());
  for (const auto& item : inputs_.items)
    item.ToProtobuf(proto->add_items());
}

void DisplayItemList::Raster(SkCanvas* canvas,
                             SkPicture::AbortCallback* callback,
                             const gfx::Rect& canvas_target_playback_rect,
                             float contents_scale) const {
  canvas->save();

  if (!canvas_target_playback_rect.IsEmpty()) {
    // canvas_target_playback_rect is specified in device space. We can't
    // use clipRect because canvas CTM will be applied on it. Use clipRegion
    // instead because it ignores canvas CTM.
    SkRegion device_clip;
    device_clip.setRect(gfx::RectToSkIRect(canvas_target_playback_rect));
    canvas->clipRegion(device_clip);
  }

  canvas->scale(contents_scale, contents_scale);
  Raster(canvas, callback);
  canvas->restore();
}

void DisplayItemList::Raster(SkCanvas* canvas,
                             SkPicture::AbortCallback* callback) const {
  if (!inputs_.settings.use_cached_picture) {
    for (const auto& item : inputs_.items)
      item.Raster(canvas, callback);
  } else {
    DCHECK(picture_);

    canvas->save();
    canvas->translate(inputs_.layer_rect.x(), inputs_.layer_rect.y());
    if (callback) {
      // If we have a callback, we need to call |draw()|, |drawPicture()|
      // doesn't take a callback.  This is used by |AnalysisCanvas| to early
      // out.
      picture_->playback(canvas, callback);
    } else {
      // Prefer to call |drawPicture()| on the canvas since it could place the
      // entire picture on the canvas instead of parsing the skia operations.
      canvas->drawPicture(picture_.get());
    }
    canvas->restore();
  }
}

void DisplayItemList::ProcessAppendedItem(const DisplayItem* item) {
  if (inputs_.settings.use_cached_picture) {
    DCHECK(recorder_);
    item->Raster(recorder_->getRecordingCanvas(), nullptr);
  }
  if (!retain_individual_display_items_) {
    inputs_.items.Clear();
  }
}

void DisplayItemList::RasterIntoCanvas(const DisplayItem& item) {
  DCHECK(recorder_);
  DCHECK(!retain_individual_display_items_);

  item.Raster(recorder_->getRecordingCanvas(), nullptr);
}

bool DisplayItemList::RetainsIndividualDisplayItems() const {
  return retain_individual_display_items_;
}

void DisplayItemList::Finalize() {
  TRACE_EVENT0("cc", "DisplayItemList::Finalize");
  // TODO(dtrainor): Need to deal with serializing visual_rects_.
  // http://crbug.com/568757.
  DCHECK(!retain_individual_display_items_ ||
         inputs_.items.size() == inputs_.visual_rects.size())
      << "items.size() " << inputs_.items.size() << " visual_rects.size() "
      << inputs_.visual_rects.size();

  // TODO(vmpstr): Build and make use of an RTree from the visual
  // rects. For now we just clear them out since we won't ever need
  // them to stick around post-Finalize. http://crbug.com/527245
  // This clears both the vector and the vector's capacity, since visual_rects_
  // won't be used anymore.
  // std::vector<gfx::Rect>().swap(inputs_.visual_rects);

  if (inputs_.settings.use_cached_picture) {
    // Convert to an SkPicture for faster rasterization.
    DCHECK(inputs_.settings.use_cached_picture);
    DCHECK(!picture_);
    picture_ = recorder_->finishRecordingAsPicture();
    DCHECK(picture_);
    picture_memory_usage_ =
        SkPictureUtils::ApproximateBytesUsed(picture_.get());
    recorder_.reset();
  }
}

bool DisplayItemList::IsSuitableForGpuRasterization() const {
  return inputs_.is_suitable_for_gpu_rasterization;
}

int DisplayItemList::ApproximateOpCount() const {
  if (retain_individual_display_items_)
    return approximate_op_count_;
  return picture_ ? picture_->approximateOpCount() : 0;
}

size_t DisplayItemList::ApproximateMemoryUsage() const {
  // We double-count in this case. Produce zero to avoid being misleading.
  if (inputs_.settings.use_cached_picture && retain_individual_display_items_)
    return 0;

  DCHECK(!inputs_.settings.use_cached_picture || picture_);

  size_t memory_usage = sizeof(*this);

  size_t external_memory_usage = 0;
  if (retain_individual_display_items_) {
    // Warning: this double-counts SkPicture data if use_cached_picture is
    // also true.
    for (const auto& item : inputs_.items) {
      external_memory_usage += item.ExternalMemoryUsage();
    }
  }

  // Memory outside this class due to |items_|.
  memory_usage += inputs_.items.GetCapacityInBytes() + external_memory_usage;

  // Memory outside this class due to |picture|.
  memory_usage += picture_memory_usage_;

  // TODO(jbroman): Does anything else owned by this class substantially
  // contribute to memory usage?

  return memory_usage;
}

bool DisplayItemList::ShouldBeAnalyzedForSolidColor() const {
  return ApproximateOpCount() <= kOpCountThatIsOkToAnalyze;
}

std::unique_ptr<base::trace_event::ConvertableToTraceFormat>
DisplayItemList::AsValue(bool include_items) const {
  std::unique_ptr<base::trace_event::TracedValue> state(
      new base::trace_event::TracedValue());

  state->BeginDictionary("params");
  if (include_items) {
    state->BeginArray("items");
    size_t item_index = 0;
    for (const DisplayItem& item : inputs_.items) {
      item.AsValueInto(item_index < inputs_.visual_rects.size()
                           ? inputs_.visual_rects[item_index]
                           : gfx::Rect(),
                       state.get());
      item_index++;
    }
    state->EndArray();  // "items".
  }
  state->SetValue("layer_rect", MathUtil::AsValue(inputs_.layer_rect));
  state->EndDictionary();  // "params".

  if (!inputs_.layer_rect.IsEmpty()) {
    SkPictureRecorder recorder;
    SkCanvas* canvas = recorder.beginRecording(inputs_.layer_rect.width(),
                                               inputs_.layer_rect.height());
    canvas->translate(-inputs_.layer_rect.x(), -inputs_.layer_rect.y());
    canvas->clipRect(gfx::RectToSkRect(inputs_.layer_rect));
    Raster(canvas, NULL, gfx::Rect(), 1.f);
    sk_sp<SkPicture> picture = recorder.finishRecordingAsPicture();

    std::string b64_picture;
    PictureDebugUtil::SerializeAsBase64(picture.get(), &b64_picture);
    state->SetString("skp64", b64_picture);
  }

  return std::move(state);
}

void DisplayItemList::EmitTraceSnapshot() const {
  TRACE_EVENT_OBJECT_SNAPSHOT_WITH_ID(
      TRACE_DISABLED_BY_DEFAULT("cc.debug.display_items") ","
      TRACE_DISABLED_BY_DEFAULT("cc.debug.picture") ","
      TRACE_DISABLED_BY_DEFAULT("devtools.timeline.picture"),
      "cc::DisplayItemList", this,
      TracedDisplayItemList::AsTraceableDisplayItemList(this,
          DisplayItemsTracingEnabled()));
}

void DisplayItemList::GenerateDiscardableImagesMetadata() {
  // This should be only called once, and only after CreateAndCacheSkPicture.
  DCHECK(image_map_.empty());
  DCHECK(!inputs_.settings.use_cached_picture || picture_);
  if (inputs_.settings.use_cached_picture && !picture_->willPlayBackBitmaps())
    return;

  // The cached picture is translated by -layer_rect_.origin during record,
  // so we need to offset that back in order to get right positioning for
  // images.
  DiscardableImageMap::ScopedMetadataGenerator generator(
      &image_map_,
      gfx::Size(inputs_.layer_rect.right(), inputs_.layer_rect.bottom()));
  Raster(generator.canvas(), nullptr,
         gfx::Rect(inputs_.layer_rect.right(), inputs_.layer_rect.bottom()),
         1.f);
}

void DisplayItemList::GetDiscardableImagesInRect(
    const gfx::Rect& rect,
    float raster_scale,
    std::vector<DrawImage>* images) {
  image_map_.GetDiscardableImagesInRect(rect, raster_scale, images);
}

void DisplayItemList::SerializeToStream(SkWStream* stream,
    PictureIdCache* old_picture_cache,
    PictureIdCache* new_picture_cache,
    bool flush_cache) const {
  stream->write32(id_);
  stream->write32(inputs_.items.size());
  DCHECK_EQ(inputs_.items.size(), inputs_.visual_rects.size());
  for (size_t i = 0; i < inputs_.items.size(); ++i) {
    stream->write32(inputs_.visual_rects[i].x());
    stream->write32(inputs_.visual_rects[i].y());
    stream->write32(inputs_.visual_rects[i].width());
    stream->write32(inputs_.visual_rects[i].height());
    sk_sp<const SkPicture> picture = inputs_.items[i].GetPicture();
    if (picture) {
      uint32_t picture_id = picture->uniqueID();
      auto it = new_picture_cache->find(picture_id);
      if (it == new_picture_cache->end()) {
        it = old_picture_cache->find(picture_id);
        if (it == old_picture_cache->end()) {
          inputs_.items[i].Serialize(stream, flush_cache);
          flush_cache = false;
        } else {
          stream->write32(DisplayItem::Drawing);
          stream->write32(picture_id);
        }
        new_picture_cache->insert(picture_id);
      } else {
        stream->write32(DisplayItem::Drawing);
        stream->write32(picture_id);
      }
    } else {
      inputs_.items[i].Serialize(stream);
    }
  }
}

sk_sp<SkData> DisplayItemList::Serialize() const {
  // TODO: not used, delete it.
  SkDynamicMemoryWStream write_stream;
  PictureIdCache old_cache;
  PictureIdCache new_cache;
  SerializeToStream(&write_stream, &old_cache, &new_cache);
  sk_sp<SkData> data(write_stream.copyToData());
  return data;
}

}  // namespace cc
