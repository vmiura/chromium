// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/playback/recording_source.h"

#include <stdint.h>

#include <algorithm>

#include "base/numerics/safe_math.h"
#include "cc/base/bulk_buffer_queue.h"
#include "cc/base/region.h"
#include "cc/layers/content_layer_client.h"
#include "cc/layers/layer_impl.h"
#include "cc/playback/display_item_list.h"
#include "cc/playback/raster_source.h"
#include "cc/proto/gfx_conversions.h"
#include "cc/proto/recording_source.pb.h"
#include "cc/trees/layer_tree_host.h"
#include "skia/ext/analysis_canvas.h"
#include "third_party/skia/include/core/SkStream.h"

namespace {

#ifdef NDEBUG
const bool kDefaultClearCanvasSetting = false;
#else
const bool kDefaultClearCanvasSetting = true;
#endif

}  // namespace

namespace cc {

namespace {

class BulkBufferWStream : public SkWStream {
 public:
  explicit BulkBufferWStream(BulkBufferWriter* writer) : writer_(writer) {
    writer_->BeginBuffer();
  }

  ~BulkBufferWStream() override {}

   bool write(const void* buffer, size_t size) override {
     DCHECK(!finalized_);
     writer_->AppendToCurrentBuffer(size, buffer);
     size_ += size;
     return true;
   }

   size_t bytesWritten() const override { return size_; }

   BulkBuffer Finalize() {
     DCHECK(!finalized_);
     finalized_ = true;
     return writer_->EndBuffer();
   }

 private:
  BulkBufferWriter* writer_;
  size_t size_ = 0;
  bool finalized_ = false;
};

class BulkBufferStream : public SkStream {
 public:
  explicit BulkBufferStream(const BulkBufferReader& reader, BulkBuffer buffer)
      : reader_(reader), view_(reader_.MakeView(std::move(buffer))) {}

  ~BulkBufferStream() override {}
    size_t read(void* buffer, size_t size) override {
      size = std::min(size, view_.size() - offset_);
      if (buffer)
        view_.Read(offset_, size, buffer);
      offset_ += size;
      return size;
    }

    size_t peek(void* buffer, size_t size) const override {
      DCHECK(buffer);
      size = std::min(size, view_.size() - offset_);
      view_.Read(offset_, size, buffer);
      return size;
    }

    bool isAtEnd() const override { return offset_ == view_.size(); }

    size_t size() const { return view_.size(); }

 private:
  const BulkBufferReader& reader_;
  const BulkBufferReader::View view_;
  size_t offset_ = 0;
};

}  // anonymous namespace

RecordingSource::RecordingSource()
    : slow_down_raster_scale_factor_for_debug_(0),
      generate_discardable_images_metadata_(false),
      requires_clear_(false),
      is_solid_color_(false),
      clear_canvas_with_debug_color_(kDefaultClearCanvasSetting),
      solid_color_(SK_ColorTRANSPARENT),
      background_color_(SK_ColorTRANSPARENT),
      last_send_display_list_id_(0),
      painter_reported_memory_usage_(0) {}

RecordingSource::~RecordingSource() {}

void RecordingSource::ToProtobuf(proto::RecordingSource* proto) const {
  RectToProto(recorded_viewport_, proto->mutable_recorded_viewport());
  SizeToProto(size_, proto->mutable_size());
  proto->set_slow_down_raster_scale_factor_for_debug(
      slow_down_raster_scale_factor_for_debug_);
  proto->set_generate_discardable_images_metadata(
      generate_discardable_images_metadata_);
  proto->set_requires_clear(requires_clear_);
  proto->set_is_solid_color(is_solid_color_);
  proto->set_clear_canvas_with_debug_color(clear_canvas_with_debug_color_);
  proto->set_solid_color(static_cast<uint64_t>(solid_color_));
  proto->set_background_color(static_cast<uint64_t>(background_color_));
  if (display_list_)
    display_list_->ToProtobuf(proto->mutable_display_list());
}

void RecordingSource::FromProtobuf(
    const proto::RecordingSource& proto,
    ClientPictureCache* client_picture_cache,
    std::vector<uint32_t>* used_engine_picture_ids) {
  DCHECK(client_picture_cache);
  recorded_viewport_ = ProtoToRect(proto.recorded_viewport());
  size_ = ProtoToSize(proto.size());
  slow_down_raster_scale_factor_for_debug_ =
      proto.slow_down_raster_scale_factor_for_debug();
  generate_discardable_images_metadata_ =
      proto.generate_discardable_images_metadata();
  requires_clear_ = proto.requires_clear();
  is_solid_color_ = proto.is_solid_color();
  clear_canvas_with_debug_color_ = proto.clear_canvas_with_debug_color();
  solid_color_ = static_cast<SkColor>(proto.solid_color());
  background_color_ = static_cast<SkColor>(proto.background_color());

  // This might not exist if the |display_list_| of the serialized
  // RecordingSource was null, wich can happen if |Clear()| is
  // called.
  if (proto.has_display_list()) {
    display_list_ = DisplayItemList::CreateFromProto(
        proto.display_list(), client_picture_cache, used_engine_picture_ids);
    FinishDisplayItemListUpdate();
  } else {
    display_list_ = nullptr;
  }
}

void RecordingSource::UpdateInvalidationForNewViewport(
    const gfx::Rect& old_recorded_viewport,
    const gfx::Rect& new_recorded_viewport,
    Region* invalidation) {
  // Invalidate newly-exposed and no-longer-exposed areas.
  Region newly_exposed_region(new_recorded_viewport);
  newly_exposed_region.Subtract(old_recorded_viewport);
  invalidation->Union(newly_exposed_region);

  Region no_longer_exposed_region(old_recorded_viewport);
  no_longer_exposed_region.Subtract(new_recorded_viewport);
  invalidation->Union(no_longer_exposed_region);
}

void RecordingSource::FinishDisplayItemListUpdate() {
  TRACE_EVENT0("cc", "RecordingSource::FinishDisplayItemListUpdate");
  DetermineIfSolidColor();
  display_list_->EmitTraceSnapshot();
  if (generate_discardable_images_metadata_)
    display_list_->GenerateDiscardableImagesMetadata();
}

void RecordingSource::SetNeedsDisplayRect(const gfx::Rect& layer_rect) {
  if (!layer_rect.IsEmpty()) {
    // Clamp invalidation to the layer bounds.
    invalidation_.Union(gfx::IntersectRects(layer_rect, gfx::Rect(size_)));
  }
}

bool RecordingSource::UpdateAndExpandInvalidation(
    ContentLayerClient* painter,
    Region* invalidation,
    const gfx::Size& layer_size,
    int frame_number,
    RecordingMode recording_mode) {
  bool updated = false;

  if (size_ != layer_size)
    size_ = layer_size;

  invalidation_.Swap(invalidation);
  invalidation_.Clear();

  gfx::Rect new_recorded_viewport = painter->PaintableRegion();
  if (new_recorded_viewport != recorded_viewport_) {
    UpdateInvalidationForNewViewport(recorded_viewport_, new_recorded_viewport,
                                     invalidation);
    recorded_viewport_ = new_recorded_viewport;
    updated = true;
  }

  if (!updated && !invalidation->Intersects(recorded_viewport_))
    return false;

  if (invalidation->IsEmpty())
    return false;

  ContentLayerClient::PaintingControlSetting painting_control =
      ContentLayerClient::PAINTING_BEHAVIOR_NORMAL;

  switch (recording_mode) {
    case RECORD_NORMALLY:
      // Already setup for normal recording.
      break;
    case RECORD_WITH_PAINTING_DISABLED:
      painting_control = ContentLayerClient::DISPLAY_LIST_PAINTING_DISABLED;
      break;
    case RECORD_WITH_CACHING_DISABLED:
      painting_control = ContentLayerClient::DISPLAY_LIST_CACHING_DISABLED;
      break;
    case RECORD_WITH_CONSTRUCTION_DISABLED:
      painting_control = ContentLayerClient::DISPLAY_LIST_CONSTRUCTION_DISABLED;
      break;
    case RECORD_WITH_SUBSEQUENCE_CACHING_DISABLED:
      painting_control = ContentLayerClient::SUBSEQUENCE_CACHING_DISABLED;
      break;
    case RECORD_WITH_SK_NULL_CANVAS:
    case RECORDING_MODE_COUNT:
      NOTREACHED();
  }

  // TODO(vmpstr): Add a slow_down_recording_scale_factor_for_debug_ to be able
  // to slow down recording.
  display_list_ = painter->PaintContentsToDisplayList(painting_control);
  painter_reported_memory_usage_ = painter->GetApproximateUnsharedMemoryUsage();

  FinishDisplayItemListUpdate();

  return true;
}

gfx::Size RecordingSource::GetSize() const {
  return size_;
}

void RecordingSource::SetEmptyBounds() {
  size_ = gfx::Size();
  Clear();
}

void RecordingSource::SetSlowdownRasterScaleFactor(int factor) {
  slow_down_raster_scale_factor_for_debug_ = factor;
}

void RecordingSource::SetGenerateDiscardableImagesMetadata(
    bool generate_metadata) {
  generate_discardable_images_metadata_ = generate_metadata;
}

void RecordingSource::SetBackgroundColor(SkColor background_color) {
  background_color_ = background_color;
}

void RecordingSource::SetRequiresClear(bool requires_clear) {
  requires_clear_ = requires_clear;
}

bool RecordingSource::IsSuitableForGpuRasterization() const {
  // The display list needs to be created (see: UpdateAndExpandInvalidation)
  // before checking for suitability. There are cases where an update will not
  // create a display list (e.g., if the size is empty). We return true in these
  // cases because the gpu suitability bit sticks false.
  return !display_list_ || display_list_->IsSuitableForGpuRasterization();
}

const DisplayItemList* RecordingSource::GetDisplayItemList() {
  return display_list_.get();
}

scoped_refptr<RasterSource> RecordingSource::CreateRasterSource(
    bool can_use_lcd_text) const {
  return scoped_refptr<RasterSource>(
      RasterSource::CreateFromRecordingSource(this, can_use_lcd_text));
}

void RecordingSource::DetermineIfSolidColor() {
  DCHECK(display_list_);
  is_solid_color_ = false;
  solid_color_ = SK_ColorTRANSPARENT;

  if (!display_list_->ShouldBeAnalyzedForSolidColor())
    return;

  TRACE_EVENT1("cc", "RecordingSource::DetermineIfSolidColor", "opcount",
               display_list_->ApproximateOpCount());
  gfx::Size layer_size = GetSize();
  skia::AnalysisCanvas canvas(layer_size.width(), layer_size.height());
  display_list_->Raster(&canvas, nullptr, gfx::Rect(), 1.f);
  is_solid_color_ = canvas.GetColorIfSolid(&solid_color_);
}

void RecordingSource::Clear() {
  recorded_viewport_ = gfx::Rect();
  display_list_ = nullptr;
  painter_reported_memory_usage_ = 0;
  is_solid_color_ = false;
}

void RecordingSource::WriteMojom(const ContentFrameBuilderContext& context,
                                 mojom::PictureLayerState* mojom) {
  TRACE_EVENT1("cc", "RecordingSource::WriteMojom", "opcount",
               display_list_ ? display_list_->ApproximateOpCount() : 0);
  if (context.flush_cache) {
    picture_id_cache_.clear();
    last_send_display_list_id_ = 0;
  }
  mojom->recorded_viewport = recorded_viewport_;
  mojom->size = size_;
  mojom->requires_clear = requires_clear_;
  mojom->is_solid_color = is_solid_color_;
  mojom->solid_color = solid_color_;
  mojom->background_color = background_color_;
  if (display_list_) {
    mojom->display_list_id = display_list_->unique_id();
    if (last_send_display_list_id_ != display_list_->unique_id()) {
      BulkBufferWStream stream(context.bulk_buffer_writer);
      {
        TRACE_EVENT0("cc", "RecordingSource::WriteMojom serialization");
        PictureIdCache new_picture_id_cache;
        display_list_->SerializeToStream(&stream, &picture_id_cache_,
                                         &new_picture_id_cache,
                                         context.flush_cache);
        picture_id_cache_ = std::move(new_picture_id_cache);
      }
      {
        TRACE_EVENT1("cc", "RecordingSource::WriteMojom finalize", "size",
                     stream.bytesWritten());
        BulkBuffer buffer = stream.Finalize();
        mojom->display_list = mojom::BulkBuffer::New();
        mojom->display_list->backings = std::move(buffer.backings);
        mojom->display_list->first_backing_begin = buffer.first_backing_begin;
        mojom->display_list->last_backing_end = buffer.last_backing_end;
      }
      last_send_display_list_id_ = display_list_->unique_id();
    }
  } else {
    mojom->display_list_id = 0;
  }
}

void RecordingSource::ReadMojom(
    const ContentFrameReaderContext& context,
    mojom::PictureLayerState* mojom,
    scoped_refptr<DisplayItemList> last_display_list,
    PictureCache* picture_cache) {
  TRACE_EVENT0("cc", "RecordingSource::ReadMojom");
  recorded_viewport_ = mojom->recorded_viewport;
  size_ = mojom->size;
  requires_clear_ = mojom->requires_clear;
  is_solid_color_ = mojom->is_solid_color;
  solid_color_ = mojom->solid_color;
  background_color_ = mojom->background_color;
  if (mojom->display_list) {
    BulkBuffer buffer;
    buffer.backings = std::move(mojom->display_list->backings);
    buffer.first_backing_begin = mojom->display_list->first_backing_begin;
    buffer.last_backing_end = mojom->display_list->last_backing_end;
    BulkBufferStream stream(context.bulk_buffer_reader, std::move(buffer));
    TRACE_EVENT1("cc", "RecordingSource::ReadMojom deserialization", "size",
                 stream.size());
    PictureCache new_picture_cache;
    display_list_ = DisplayItemList::CreateFromStream(
        &stream, picture_cache, &new_picture_cache);
    *picture_cache = std::move(new_picture_cache);
  } else {
    if (last_display_list &&
        last_display_list->unique_id() == mojom->display_list_id)
      display_list_ = std::move(last_display_list);
  }
}

}  // namespace cc
