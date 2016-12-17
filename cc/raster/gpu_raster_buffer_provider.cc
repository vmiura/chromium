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
#include "gpu/command_buffer/client/context_support.h"
#include "third_party/skia/include/core/SkWriteBuffer.h"
#include "third_party/skia/include/core/SkFlattenableSerialization.h"
#include "third_party/skia/include/core/SkMultiPictureDraw.h"
#include "third_party/skia/include/core/SkPictureRecorder.h"
#include "third_party/skia/include/core/SkSurface.h"
#include "third_party/skia/include/gpu/GrContext.h"
#include "third_party/skia/src/core/SkMatrixUtils.h"

namespace cc {
namespace {

static unsigned gPictureImageKeyNamespaceLabel;

struct PictureImageKey : public gpu::CdlResourceCache::Key {
public:
  PictureImageKey(uint32_t pictureID,
                  const SkRect& tile,
                  SkShader::TileMode tmx,
                  SkShader::TileMode tmy,
                  const SkSize& scale,
                  const SkMatrix& localMatrix)
    : fPictureID(pictureID)
    , fTile(tile)
    , fTmx(tmx)
    , fTmy(tmy)
    , fScale(scale) {

    for (int i = 0; i < 9; ++i) {
      fLocalMatrixStorage[i] = localMatrix[i];
    }

    static const size_t keySize = sizeof(fPictureID) +
                                  sizeof(fTile) +
                                  sizeof(fTmx) + sizeof(fTmy) +
                                  sizeof(fScale) +
                                  sizeof(fLocalMatrixStorage);
    // This better be packed.
    SkASSERT(sizeof(uint32_t) * (&fEndOfStruct - &fPictureID) == keySize);
    this->init(&gPictureImageKeyNamespaceLabel, keySize);
  }

private:
  uint32_t           fPictureID;
  SkRect             fTile;
  SkShader::TileMode fTmx, fTmy;
  SkSize             fScale;
  SkScalar           fLocalMatrixStorage[9];

  SkDEBUGCODE(uint32_t fEndOfStruct;)
};

struct PictureImageRec : public gpu::CdlResourceCache::Rec {
  PictureImageRec(const PictureImageKey& key, sk_sp<SkImage> image)
      : key(key)
      , image(image) {}

  PictureImageKey key;
  sk_sp<SkImage>  image;

  const Key& getKey() const override { return key; }
  size_t bytesUsed() const override {
      // Just the record overhead -- the actual pixels are accounted by SkImageCacherator.
      return sizeof(key) + sizeof(image);
  }
  const char* getCategory() const override { return "picture-image"; }

  static bool Visitor(const gpu::CdlResourceCache::Rec& baseRec, void* contextShader) {
      const PictureImageRec& rec = static_cast<const PictureImageRec&>(baseRec);
      SkImage** result = reinterpret_cast<SkImage**>(contextShader);
      *result = rec.image.get();
      return true;
  }
};

class SkCommandBufferCanvas : public SkNoDrawCanvas {
 public:
  SkCommandBufferCanvas(int width, int height, gpu::gles2::GLES2Interface* gl,
                        gpu::ContextSupport* context_support)
      : SkNoDrawCanvas(width, height),
        gl_(gl),
        context_support_(context_support) {}
  ~SkCommandBufferCanvas() override {}

 protected:
  void onDrawPicture(const SkPicture* picture,
                     const SkMatrix* matrix,
                     const SkPaint* paint) override {
    SkCanvas::onDrawPicture(picture, matrix, paint);
  }

  SkCanvas::SaveLayerStrategy getSaveLayerStrategy(
      const SaveLayerRec& rec) override {
    gl_->CanvasSaveLayer(rec.fBounds, rec.fPaint, rec.fBackdrop,
                         rec.fSaveLayerFlags);
    return SkCanvas::kNoLayer_SaveLayerStrategy;
  }

  void willSave() override { gl_->CanvasSave(); }

  void didRestore() override { gl_->CanvasRestore(); }

  void didConcat(const SkMatrix& mat) override {
    float m[9];
    mat.get9(m);
    gl_->CanvasSetMatrix(true, m);
  }
  void didSetMatrix(const SkMatrix& mat) override {
    float m[9];
    mat.get9(m);
    gl_->CanvasSetMatrix(false, m);
  }
  void didTranslate(SkScalar tx, SkScalar ty) override {
    gl_->CanvasTranslate(tx, ty);
  }

  void onClipRect(const SkRect& r,
                  SkRegion::Op op,
                  SkCanvas::ClipEdgeStyle style) override {
    SkNoDrawCanvas::onClipRect(r, op, style);
    gl_->CanvasClipRect(r, (unsigned)op, style == kSoft_ClipEdgeStyle);
  }

  void onClipRRect(const SkRRect& r,
                   SkRegion::Op op,
                   SkCanvas::ClipEdgeStyle style) override {
    SkNoDrawCanvas::onClipRRect(r, op, style);
    gl_->CanvasClipRRect(r, (unsigned)op, style == kSoft_ClipEdgeStyle);
  }

  void onClipPath(const SkPath& p,
                  SkRegion::Op op,
                  SkCanvas::ClipEdgeStyle style) override {
    SkNoDrawCanvas::onClipPath(p, op, style);
    gl_->CanvasClipPath(p, (unsigned)op, style == kSoft_ClipEdgeStyle);
  }

  void onClipRegion(const SkRegion& r,
                    SkRegion::Op op) override {
    SkNoDrawCanvas::onClipRegion(r, op);
    gl_->CanvasClipRegion(r, (unsigned)op);
  }

  void onDrawPaint(SkPaint const& paint) override {
    if (paint.getShader())
      SetupShader(paint.getShader());
    gl_->CanvasDrawPaint(paint);
  }

  void onDrawRect(const SkRect& r, const SkPaint& paint) override {
    if (paint.getShader())
      SetupShader(paint.getShader());
    gl_->CanvasDrawRect(r, paint);
  }

  void onDrawOval(const SkRect& r, const SkPaint& paint) override {
    if (paint.getShader())
      SetupShader(paint.getShader());
    gl_->CanvasDrawOval(r, paint);
  }

  void onDrawRRect(const SkRRect& r, const SkPaint& paint) override {
    if (paint.getShader())
      SetupShader(paint.getShader());
    gl_->CanvasDrawRRect(r, paint);
  }

  void onDrawPath(const SkPath& path, const SkPaint& paint) override {
    if (paint.getShader())
      SetupShader(paint.getShader());
    gl_->CanvasDrawPath(path, paint);
  }

  void onDrawTextBlob(const SkTextBlob* blob,
                      SkScalar x,
                      SkScalar y,
                      const SkPaint& paint) override {
    if (paint.getShader())
      SetupShader(paint.getShader());
    gl_->CanvasDrawTextBlob(blob, x, y, paint);
  }

  void onDrawImage(const SkImage* image,
                   SkScalar left,
                   SkScalar top,
                   const SkPaint* paint) override {
    gl_->CanvasDrawImage(image, left, top, paint);
  }

  void onDrawImageRect(const SkImage* image,
                       const SkRect* src,
                       const SkRect& dst,
                       const SkPaint* paint,
                       SkCanvas::SrcRectConstraint constraint) override {
    gl_->CanvasDrawImageRect(image, src, dst, paint,
                             constraint == SkCanvas::kStrict_SrcRectConstraint);
  }
 private:
  void SetupShader(const SkShader* shader) {
    SkShader::GradientType type = shader->asAGradient(0);
    if (type != SkShader::kNone_GradientType) {
      gl_->CanvasSetGradientShader(shader);
    } else if (shader->isAImage()) {
      SkMatrix local_matrix;
      SkShader::TileMode tile_mode[2];
      SkImage* image = shader->isAImage(&local_matrix, tile_mode);
      gl_->CanvasSetImageShader(image, tile_mode[0], tile_mode[1], &local_matrix);
    } else if (shader->isAPicture()) {
      SkMatrix local_matrix;
      SkShader::TileMode tile_mode[2];
      SkRect tile;
      SkPicture* picture = shader->isAPicture(&local_matrix, tile_mode, &tile);

      // TODO(cdl): Figure out where this optional local matrix comes from.
      SkMatrix* localM = nullptr;
      SkMatrix m;
      m.setConcat(getTotalMatrix(), local_matrix);
      if (localM) {
          m.preConcat(*localM);
      }

      // Use a rotation-invariant scale
      SkPoint scale;
      //
      // TODO: replace this with decomposeScale() -- but beware LayoutTest rebaselines!
      //
      if (!SkDecomposeUpper2x2(m, nullptr, &scale, nullptr)) {
          // Decomposition failed, use an approximation.
          scale.set(SkScalarSqrt(m.getScaleX() * m.getScaleX() + m.getSkewX() * m.getSkewX()),
                    SkScalarSqrt(m.getScaleY() * m.getScaleY() + m.getSkewY() * m.getSkewY()));
      }
      SkSize scaledSize = SkSize::Make(SkScalarAbs(scale.x() * tile.width()),
                                       SkScalarAbs(scale.y() * tile.height()));

      // Clamp the tile size to about 4M pixels
      static const SkScalar kMaxTileArea = 2048 * 2048;
      SkScalar tileArea = SkScalarMul(scaledSize.width(), scaledSize.height());
      if (tileArea > kMaxTileArea) {
          SkScalar clampScale = SkScalarSqrt(kMaxTileArea / tileArea);
          scaledSize.set(SkScalarMul(scaledSize.width(), clampScale),
                         SkScalarMul(scaledSize.height(), clampScale));
      }
  #if SK_SUPPORT_GPU
      // Scale down the tile size if larger than maxTextureSize for GPU Path or it should fail on create texture
      int maxTextureSize = 2048;
      if (maxTextureSize) {
          if (scaledSize.width() > maxTextureSize || scaledSize.height() > maxTextureSize) {
              SkScalar downScale = maxTextureSize / SkMaxScalar(scaledSize.width(), scaledSize.height());
              scaledSize.set(SkScalarFloorToScalar(SkScalarMul(scaledSize.width(), downScale)),
                             SkScalarFloorToScalar(SkScalarMul(scaledSize.height(), downScale)));
          }
      }
  #endif

  #ifdef SK_SUPPORT_LEGACY_PICTURESHADER_ROUNDING
      const SkISize tileSize = scaledSize.toRound();
  #else
      const SkISize tileSize = scaledSize.toCeil();
  #endif
      if (tileSize.isEmpty()) {
        // TODO: Make empty shader if needed.
        //return SkShader::MakeEmptyShader();
        return;
      }

      // The actual scale, compensating for rounding & clamping.
      const SkSize tileScale = SkSize::Make(SkIntToScalar(tileSize.width()) / tile.width(),
                                            SkIntToScalar(tileSize.height()) / tile.height());

      SkImage* tile_image = 0;
      PictureImageKey key(picture->uniqueID(),
                          tile,
                          tile_mode[0],
                          tile_mode[1],
                          tileScale,
                          local_matrix);

      gpu::CdlResourceCache* resource_cache = context_support_->resource_cache();
      if (!resource_cache->find(key, PictureImageRec::Visitor, &tile_image)) {
        // Temp: Draw SkPicture to a local SkBitmap.
        SkBitmap bitmap;
        bitmap.allocPixels(
            SkImageInfo::MakeN32Premul(tileSize.width(), tileSize.height()));
        SkCanvas canvas(bitmap);
        canvas.drawPicture(picture);
        bitmap.setImmutable();
        sk_sp<SkImage> image = SkImage::MakeFromBitmap(bitmap);

        // Save image in resource cache for re-use.
        resource_cache->add(new PictureImageRec(key, image));

        gl_->CanvasSetImageShader(image.get(), tile_mode[0], tile_mode[1], &local_matrix);
      } else {
        gl_->CanvasSetImageShader(tile_image, tile_mode[0], tile_mode[1], &local_matrix);
      }
    }
  }

  gpu::gles2::GLES2Interface* gl_;
  gpu::ContextSupport* context_support_;
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

#define USE_CANVAS_COMMAND_BUFFER 1

#if USE_CANVAS_COMMAND_BUFFER
  ResourceProvider::ScopedCdlSurfaceProvider scoped_surface(
      context_provider, resource_lock, async_worker_context_enabled,
      use_distance_field_text, raster_source->CanUseLCDText(),
      raster_source->HasImpliedColorSpace(), msaa_sample_count);

  SkCommandBufferCanvas canvas(resource_size.width(),
                               resource_size.height(),
                               context_provider->ContextGL(),
                               context_provider->ContextSupport());

  raster_source->PlaybackToCanvas(&canvas, raster_full_rect, playback_rect,
                                  scales, playback_settings);
#else
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

  raster_source->PlaybackToCanvas(sk_surface->getCanvas(), raster_full_rect,
                                playback_rect, scales, playback_settings);
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
  //return msaa_sample_count_ == 0;
  return false;
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
