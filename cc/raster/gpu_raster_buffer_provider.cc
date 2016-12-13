// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/raster/gpu_raster_buffer_provider.h"

#include <stdint.h>

#include <algorithm>

#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/metrics/histogram_macros.h"
#include "base/trace_event/trace_event.h"
#include "cc/base/histograms.h"
#include "cc/playback/image_hijack_canvas.h"
#include "cc/playback/raster_source.h"
#include "cc/raster/scoped_gpu_raster.h"
#include "cc/resources/resource.h"
#include "gpu/command_buffer/client/gles2_interface.h"
#include "skia/ext/cdl_canvas.h"
#include "skia/ext/cdl_no_draw_canvas.h"
#include "skia/ext/cdl_paint.h"
#include "skia/ext/cdl_picture.h"
#include "skia/ext/cdl_picture_recorder.h"
#include "third_party/skia/include/core/SkWriteBuffer.h"
#include "third_party/skia/include/core/SkFlattenableSerialization.h"
#include "third_party/skia/include/core/SkMultiPictureDraw.h"
#include "third_party/skia/include/core/SkPictureRecorder.h"
#include "third_party/skia/include/core/SkSurface.h"
#include "third_party/skia/include/gpu/GrContext.h"

namespace cc {
namespace {

class CdlCommandBufferCanvas : public CdlNoDrawCanvas {
 public:
  CdlCommandBufferCanvas(int width, int height, gpu::gles2::GLES2Interface* gl)
      : CdlNoDrawCanvas(width, height), gl_(gl) {}
  ~CdlCommandBufferCanvas() override {}

 protected:
  inline uint32_t GetPaintBits(const CdlPaint& paint) {
    CdlPaintBits paint_bits;
    paint_bits.bitfields.flags = paint.getFlags();
    paint_bits.bitfields.text_align = 0;
    paint_bits.bitfields.cap_type = paint.getStrokeCap();
    paint_bits.bitfields.join_type = paint.getStrokeJoin();
    paint_bits.bitfields.style = paint.getStyle();
    paint_bits.bitfields.text_encoding = paint.getTextEncoding();
    paint_bits.bitfields.hinting = paint.getHinting();
    paint_bits.bitfields.filter_quality = paint.getFilterQuality();
    return paint_bits.bitfields_uint;
  }

  void onDrawPicture(const CdlPicture* picture,
                     const SkMatrix* matrix,
                     const CdlPaint* paint) override {
    CdlCanvas::onDrawPicture(picture, matrix, paint);
  }

  int onSaveLayer(const SaveLayerRec& rec) override {
    gl_->CanvasSaveLayer(rec.fBounds, rec.fPaint, rec.fBackdrop,
                         rec.fSaveLayerFlags);
    return CdlNoDrawCanvas::onSaveLayer(rec);
  }

  int onSave() override {
    gl_->CanvasSave();
    return CdlNoDrawCanvas::onSave();
  }

  void onRestore() override {
    gl_->CanvasRestore();
    CdlNoDrawCanvas::onRestore();
  }

  void onConcat(const SkMatrix& mat) override {
    float m[9];
    mat.get9(m);
    gl_->CanvasSetMatrix(true, m);
    CdlNoDrawCanvas::onConcat(mat);
  }
  void onSetMatrix(const SkMatrix& mat) override {
    float m[9];
    mat.get9(m);
    gl_->CanvasSetMatrix(false, m);
    CdlNoDrawCanvas::onSetMatrix(mat);
  }
  void onTranslate(SkScalar tx, SkScalar ty) override {
    gl_->CanvasTranslate(tx, ty);
    CdlNoDrawCanvas::onTranslate(tx, ty);
  }

  void onClipRect(const SkRect& r,
                  SkRegion::Op op,
                  CdlCanvas::ClipEdgeStyle style) override {
    gl_->CanvasClipRect(r, (unsigned)op, style == kSoft_ClipEdgeStyle);
    CdlNoDrawCanvas::onClipRect(r, op, style);
  }

  void onClipRRect(const SkRRect& r,
                   SkRegion::Op op,
                   CdlCanvas::ClipEdgeStyle style) override {
    gl_->CanvasClipRRect(r, (unsigned)op, style == kSoft_ClipEdgeStyle);
    CdlNoDrawCanvas::onClipRRect(r, op, style);
  }

  void onClipPath(const SkPath& p,
                   SkRegion::Op op,
                   CdlCanvas::ClipEdgeStyle style) override {
    gl_->CanvasClipPath(p, (unsigned)op, style == kSoft_ClipEdgeStyle);
    CdlNoDrawCanvas::onClipPath(p, op, style);
  }

  void onDrawPaint(CdlPaint const& paint) override {
    if (paint.getShader())
      paint.getShader()->setupShader(gl_);
    gl_->CanvasDrawPaint(paint.getStrokeWidth(), paint.getStrokeMiter(),
                         paint.getColor(), (unsigned)paint.getBlendMode(),
                         GetPaintBits(paint));
  }

  void onDrawRect(const SkRect& r, const CdlPaint& paint) override {
    if (paint.getShader())
      paint.getShader()->setupShader(gl_);
    gl_->CanvasDrawRect(r.left(), r.top(), r.right(), r.bottom(),
                        paint.getStrokeWidth(), paint.getStrokeMiter(),
                        paint.getColor(), (unsigned)paint.getBlendMode(),
                        GetPaintBits(paint));
  }

  // void setRectRadii(const SkRect& rect, const SkVector radii[4]);

  void onDrawRRect(const SkRRect& r, const CdlPaint& paint) override {
    if (paint.getShader())
      paint.getShader()->setupShader(gl_);
    gl_->CanvasDrawRRect(
        r.rect().left(), r.rect().top(), r.rect().right(), r.rect().bottom(),
        r.radii(SkRRect::kUpperLeft_Corner).x(),
        r.radii(SkRRect::kUpperLeft_Corner).y(),
        r.radii(SkRRect::kUpperRight_Corner).x(),
        r.radii(SkRRect::kUpperRight_Corner).y(),
        r.radii(SkRRect::kLowerRight_Corner).x(),
        r.radii(SkRRect::kLowerRight_Corner).y(),
        r.radii(SkRRect::kLowerLeft_Corner).x(),
        r.radii(SkRRect::kLowerLeft_Corner).y(), paint.getStrokeWidth(),
        paint.getStrokeMiter(), paint.getColor(),
        (unsigned)paint.getBlendMode(), GetPaintBits(paint));
  }

  void onDrawPath(const SkPath& path,
                  const CdlPaint& paint) override {
    gl_->CanvasDrawPath(path, paint);
  }

  void onDrawImage(const SkImage* image,
                   SkScalar left,
                   SkScalar top,
                   const CdlPaint* paint) override {
    gl_->CanvasDrawImage(image, left, top, paint);
  }

  void onDrawImageRect(const SkImage* image,
                       const SkRect* src,
                       const SkRect& dst,
                       const CdlPaint* paint,
                       SkCanvas::SrcRectConstraint constraint) override {
    gl_->CanvasDrawImageRect(image, src, dst, paint,
                             constraint == SkCanvas::kStrict_SrcRectConstraint);
  }

  void onDrawTextBlob(const SkTextBlob* blob,
                      SkScalar x,
                      SkScalar y,
                      const CdlPaint& paint) override {
    if (paint.getShader())
      paint.getShader()->setupShader(gl_);
    gl_->CanvasDrawTextBlob(blob, x, y, paint);
  }

  gpu::gles2::GLES2Interface* gl_;
};

static void RasterizeSource(
    const RasterSource* raster_source,
    bool resource_has_previous_content,
    const gfx::Size& resource_size,
    const gfx::Rect& raster_full_rect,
    const gfx::Rect& raster_dirty_rect,
    const gfx::SizeF& scales,
    const RasterSource::PlaybackSettings& playback_settings,
    ContextProvider* context_provider,
    ResourceProvider::ScopedWriteLockGL* resource_lock,
    bool async_worker_context_enabled,
    bool use_distance_field_text,
    int msaa_sample_count) {
  ResourceProvider::ScopedCdlSurfaceProvider scoped_surface(
      context_provider, resource_lock, async_worker_context_enabled,
      use_distance_field_text, raster_source->CanUseLCDText(),
      raster_source->HasImpliedColorSpace(), msaa_sample_count);

  // context_provider->ContextGL()->CdlDrawRectangle(10, 10, 23, 23,
  // 0x23232323);

  CdlCommandBufferCanvas canvas(resource_size.width(), resource_size.height(),
                                context_provider->ContextGL());

#if 0
  ScopedGpuRaster gpu_raster(context_provider);

  ResourceProvider::ScopedSkSurfaceProvider scoped_surface(
      context_provider, resource_lock, async_worker_context_enabled,
      use_distance_field_text, raster_source->CanUseLCDText(),
      raster_source->HasImpliedColorSpace(), msaa_sample_count);
  SkSurface* sk_surface = scoped_surface.sk_surface();
  // Allocating an SkSurface will fail after a lost context.  Pretend we
  // rasterized, as the contents of the resource don't matter anymore.
  if (!sk_surface)
    return;
#endif

  // Playback
  gfx::Rect playback_rect = raster_full_rect;
  if (resource_has_previous_content) {
    playback_rect.Intersect(raster_dirty_rect);
  }
  DCHECK(!playback_rect.IsEmpty())
      << "Why are we rastering a tile that's not dirty?";

  // Log a histogram of the percentage of pixels that were saved due to
  // partial raster.
  const char* client_name = GetClientNameForMetrics();
  float full_rect_size = raster_full_rect.size().GetArea();
  if (full_rect_size > 0 && client_name) {
    float fraction_partial_rastered =
        static_cast<float>(playback_rect.size().GetArea()) / full_rect_size;
    float fraction_saved = 1.0f - fraction_partial_rastered;
    UMA_HISTOGRAM_PERCENTAGE(
        base::StringPrintf("Renderer4.%s.PartialRasterPercentageSaved.Gpu",
                           client_name),
        100.0f * fraction_saved);
  }

#if 0
  raster_source->PlaybackToCanvas(sk_surface->getCanvas(), raster_full_rect,
                                  playback_rect, scales, playback_settings);
#else
  raster_source->PlaybackToCanvas(&canvas, raster_full_rect, playback_rect,
                                  scales, playback_settings);
#endif
}

}  // namespace

GpuRasterBufferProvider::RasterBufferImpl::RasterBufferImpl(
    GpuRasterBufferProvider* client,
    ResourceProvider* resource_provider,
    ResourceId resource_id,
    bool async_worker_context_enabled,
    bool resource_has_previous_content)
    : client_(client),
      lock_(resource_provider, resource_id, async_worker_context_enabled),
      resource_has_previous_content_(resource_has_previous_content) {
  client_->pending_raster_buffers_.insert(this);
}

GpuRasterBufferProvider::RasterBufferImpl::~RasterBufferImpl() {
  client_->pending_raster_buffers_.erase(this);
}

void GpuRasterBufferProvider::RasterBufferImpl::Playback(
    const RasterSource* raster_source,
    const gfx::Rect& raster_full_rect,
    const gfx::Rect& raster_dirty_rect,
    uint64_t new_content_id,
    const gfx::SizeF& scales,
    const RasterSource::PlaybackSettings& playback_settings) {
  TRACE_EVENT0("cc", "GpuRasterBuffer::Playback");
  client_->PlaybackOnWorkerThread(&lock_, sync_token_,
                                  resource_has_previous_content_, raster_source,
                                  raster_full_rect, raster_dirty_rect,
                                  new_content_id, scales, playback_settings);
}

GpuRasterBufferProvider::GpuRasterBufferProvider(
    ContextProvider* compositor_context_provider,
    ContextProvider* worker_context_provider,
    ResourceProvider* resource_provider,
    bool use_distance_field_text,
    int gpu_rasterization_msaa_sample_count,
    bool async_worker_context_enabled)
    : compositor_context_provider_(compositor_context_provider),
      worker_context_provider_(worker_context_provider),
      resource_provider_(resource_provider),
      use_distance_field_text_(use_distance_field_text),
      msaa_sample_count_(gpu_rasterization_msaa_sample_count),
      async_worker_context_enabled_(async_worker_context_enabled) {
  DCHECK(compositor_context_provider);
  DCHECK(worker_context_provider);
}

GpuRasterBufferProvider::~GpuRasterBufferProvider() {
  DCHECK(pending_raster_buffers_.empty());
}

std::unique_ptr<RasterBuffer> GpuRasterBufferProvider::AcquireBufferForRaster(
    const Resource* resource,
    uint64_t resource_content_id,
    uint64_t previous_content_id) {
  bool resource_has_previous_content =
      resource_content_id && resource_content_id == previous_content_id;
  return base::MakeUnique<RasterBufferImpl>(
      this, resource_provider_, resource->id(), async_worker_context_enabled_,
      resource_has_previous_content);
}

void GpuRasterBufferProvider::ReleaseBufferForRaster(
    std::unique_ptr<RasterBuffer> buffer) {
  // Nothing to do here. RasterBufferImpl destructor cleans up after itself.
}

void GpuRasterBufferProvider::OrderingBarrier() {
  TRACE_EVENT0("cc", "GpuRasterBufferProvider::OrderingBarrier");

  gpu::gles2::GLES2Interface* gl = compositor_context_provider_->ContextGL();
  if (async_worker_context_enabled_) {
    GLuint64 fence = gl->InsertFenceSyncCHROMIUM();
    gl->OrderingBarrierCHROMIUM();

    gpu::SyncToken sync_token;
    gl->GenUnverifiedSyncTokenCHROMIUM(fence, sync_token.GetData());

    DCHECK(sync_token.HasData() ||
           gl->GetGraphicsResetStatusKHR() != GL_NO_ERROR);

    for (RasterBufferImpl* buffer : pending_raster_buffers_)
      buffer->set_sync_token(sync_token);
  } else {
    gl->OrderingBarrierCHROMIUM();
  }
  pending_raster_buffers_.clear();
}

ResourceFormat GpuRasterBufferProvider::GetResourceFormat(
    bool must_support_alpha) const {
  return resource_provider_->best_render_buffer_format();
}

bool GpuRasterBufferProvider::IsResourceSwizzleRequired(
    bool must_support_alpha) const {
  // This doesn't require a swizzle because we rasterize to the correct format.
  return false;
}

bool GpuRasterBufferProvider::CanPartialRasterIntoProvidedResource() const {
  // Partial raster doesn't support MSAA, as the MSAA resolve is unaware of clip
  // rects.
  // TODO(crbug.com/629683): See if we can work around this limitation.
  return msaa_sample_count_ == 0;
}

void GpuRasterBufferProvider::Shutdown() {
  pending_raster_buffers_.clear();
}

void GpuRasterBufferProvider::PlaybackOnWorkerThread(
    ResourceProvider::ScopedWriteLockGL* resource_lock,
    const gpu::SyncToken& sync_token,
    bool resource_has_previous_content,
    const RasterSource* raster_source,
    const gfx::Rect& raster_full_rect,
    const gfx::Rect& raster_dirty_rect,
    uint64_t new_content_id,
    const gfx::SizeF& scales,
    const RasterSource::PlaybackSettings& playback_settings) {
  ContextProvider::ScopedContextLock scoped_context(worker_context_provider_);
  gpu::gles2::GLES2Interface* gl = scoped_context.ContextGL();
  DCHECK(gl);

  if (async_worker_context_enabled_) {
    // Early out if sync token is invalid. This happens if the compositor
    // context was lost before ScheduleTasks was called.
    if (!sync_token.HasData())
      return;
    // Synchronize with compositor.
    gl->WaitSyncTokenCHROMIUM(sync_token.GetConstData());
  }

#if 1
  // Turn on distance fields for layers that have ever animated.
  bool use_distance_field_text =
      use_distance_field_text_ ||
      raster_source->ShouldAttemptToUseDistanceFieldText();

  RasterizeSource(raster_source, resource_has_previous_content,
                  resource_lock->size(), raster_full_rect, raster_dirty_rect,
                  scales, playback_settings, worker_context_provider_,
                  resource_lock, async_worker_context_enabled_,
                  use_distance_field_text, msaa_sample_count_);
#endif

  const uint64_t fence_sync = gl->InsertFenceSyncCHROMIUM();

  // Barrier to sync worker context output to cc context.
  gl->OrderingBarrierCHROMIUM();

  // Generate sync token after the barrier for cross context synchronization.
  gpu::SyncToken resource_sync_token;
  gl->GenUnverifiedSyncTokenCHROMIUM(fence_sync, resource_sync_token.GetData());
  resource_lock->set_sync_token(resource_sync_token);
  resource_lock->set_synchronized(!async_worker_context_enabled_);
}

}  // namespace cc
