#include "cmd_buffer_canvas.h"

#include "base/logging.h"
#include "gpu/command_buffer/client/gles2_interface.h"
#include "gpu/command_buffer/client/context_support.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkImageFilter.h"
#include "third_party/skia/include/core/SkPicture.h"
#include "third_party/skia/include/utils/SkNoDrawCanvas.h"
#include "third_party/skia/src/core/SkMatrixUtils.h"

namespace gpu {

namespace {

static unsigned gPictureImageKeyNamespaceLabel;

struct PictureImageKey : public gpu::CdlResourceCache::Key {
public:
  PictureImageKey(uint32_t picture_id,
                  const SkRect& tile,
                  const SkISize& tile_size)
    : picture_id(picture_id)
    , tile(tile)
    , tile_size(tile_size) {

    static const size_t keySize = sizeof(this->picture_id) +
                                  sizeof(this->tile) +
                                  sizeof(this->tile_size);
    // This better be packed.
    SkASSERT(sizeof(uint32_t) * (&fEndOfStruct - &fPictureID) == keySize);
    this->init(&gPictureImageKeyNamespaceLabel, keySize);
  }

private:
  uint32_t           picture_id;
  SkRect             tile;
  SkISize            tile_size;

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

SkISize ComputePictureImageSize(SkMatrix &m, SkRect tile) {
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

  return scaledSize.toCeil();
}

} // anon namespace

class CommandBufferCanvas : public SkNoDrawCanvas {
 public:
  CommandBufferCanvas(int width, int height, gpu::gles2::GLES2Interface* gl,
                        gpu::ContextSupport* context_support)
      : SkNoDrawCanvas(width, height),
        gl_(gl),
        context_support_(context_support) {}
  ~CommandBufferCanvas() override {}

 protected:
  void onDrawPicture(const SkPicture* picture,
                     const SkMatrix* matrix,
                     const SkPaint* paint) override {
    SkCanvas::onDrawPicture(picture, matrix, paint);
  }

  SkCanvas::SaveLayerStrategy getSaveLayerStrategy(
      const SaveLayerRec& rec) override {
    if (rec.fPaint) SetupPaint(*rec.fPaint);
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
    SetupPaint(paint);
    gl_->CanvasDrawPaint(paint);
  }

  void onDrawRect(const SkRect& r, const SkPaint& paint) override {
    SetupPaint(paint);
    gl_->CanvasDrawRect(r, paint);
  }

  void onDrawOval(const SkRect& r, const SkPaint& paint) override {
    SetupPaint(paint);
    gl_->CanvasDrawOval(r, paint);
  }

  void onDrawRRect(const SkRRect& r, const SkPaint& paint) override {
    SetupPaint(paint);
    gl_->CanvasDrawRRect(r, paint);
  }

  void onDrawPath(const SkPath& path, const SkPaint& paint) override {
    SetupPaint(paint);
    gl_->CanvasDrawPath(path, paint);
  }

  void onDrawTextBlob(const SkTextBlob* blob,
                      SkScalar x,
                      SkScalar y,
                      const SkPaint& paint) override {
    SetupPaint(paint);
    gl_->CanvasDrawTextBlob(blob, x, y, paint);
  }

  void onDrawImage(const SkImage* image,
                   SkScalar left,
                   SkScalar top,
                   const SkPaint* paint) override {
    if (paint) SetupPaint(*paint);
    gl_->CanvasDrawImage(image, left, top, paint);
  }

  void onDrawImageRect(const SkImage* image,
                       const SkRect* src,
                       const SkRect& dst,
                       const SkPaint* paint,
                       SkCanvas::SrcRectConstraint constraint) override {
    if (paint) SetupPaint(*paint);
    gl_->CanvasDrawImageRect(image, src, dst, paint,
                             constraint == SkCanvas::kStrict_SrcRectConstraint);
  }

 private:
  void SetupPaint(const SkPaint& paint) {
    if (paint.getShader())
      SetupShader(paint.getShader());
    if (paint.getImageFilter())
      SetupImageFilter(paint.getImageFilter(), 0, 0);
  }

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

      // TODO: Figure out where this optional local matrix comes from.
      SkMatrix* localM = nullptr;
      SkMatrix full_matrix;
      full_matrix.setConcat(getTotalMatrix(), local_matrix);
      if (localM) {
          full_matrix.preConcat(*localM);
      }

      const SkISize tileSize = ComputePictureImageSize(full_matrix, tile);
  
      // TODO: Make empty shader if needed.
      if (tileSize.isEmpty()) 
        return;

      SkImage* tile_image = 0;
      PictureImageKey key(picture->uniqueID(), tile, tileSize);

      gpu::CdlResourceCache* resource_cache = context_support_->resource_cache();
      if (!resource_cache->find(key, PictureImageRec::Visitor, &tile_image)) {
        // Temp: Draw SkPicture to a local SkBitmap.
        SkBitmap bitmap;
        bitmap.allocPixels(
            SkImageInfo::MakeN32Premul(tileSize.width(), tileSize.height()));
        bitmap.eraseColor(SK_ColorTRANSPARENT);
        SkCanvas canvas(bitmap, SkSurfaceProps(0, kUnknown_SkPixelGeometry));

        SkMatrix tileMatrix;
        tileMatrix.setRectToRect(tile, SkRect::MakeIWH(tileSize.width(), tileSize.height()),
                                 SkMatrix::kFill_ScaleToFit);

        canvas.drawPicture(picture, &tileMatrix, nullptr);
        bitmap.setImmutable();
        sk_sp<SkImage> image = SkImage::MakeFromBitmap(bitmap);

        // Save image in resource cache for re-use.
        resource_cache->add(new PictureImageRec(key, image));
        tile_image = image.get();
      }

      if (tile_image) {
        SkMatrix shader_matrix = local_matrix;
        shader_matrix.preScale(tile.width() / SkIntToScalar(tileSize.width()),
                               tile.height() / SkIntToScalar(tileSize.height()));
        gl_->CanvasSetImageShader(tile_image, tile_mode[0], tile_mode[1],
                                  &shader_matrix);
      }
    }
  }

  void SetupImageFilter(const SkImageFilter* filter, int index, int input) {
    const char* filter_type = "NULL"; 
    if (filter) {
      //int num_inputs = filter->countInputs();
      //for (int i = 0; i < num_inputs; i++) {
      //  SkImageFilter* input = filter->getInput(i);
      //  if (input)  
      //    SetupImageFilter(input, index + 1, i);
      //}

      if (filter->isColorFilterNode(nullptr)) {
        filter_type = "COLOR_FILTER";
        SkColorFilter* color_filter = nullptr;
        filter->isColorFilterNode(&color_filter);
        gl_->CanvasSetColorFilter(color_filter);
      }
      else if (filter->isBlurFilterNode(nullptr, nullptr)) {
        filter_type = "BLUR_FILTER";
        SkScalar sigma_x, sigma_y;
        filter->isBlurFilterNode(&sigma_x, &sigma_y);
        gl_->CanvasSetBlurFilter(sigma_x, sigma_y, false);
      } else if (filter->isImageSourceNode(0, 0, 0)) {
        filter_type = "IMAGE_SRC";
      } else if (filter->isPictureFilterNode(0, 0, 0)) {
        filter_type = "PICTURE_FILTER";
      } else {
        filter_type = "????";
      }
    }

    //LOG(ERROR) << "filter index " << index
    //          << " input " << input
    //          << " " << filter_type;
  }

  gpu::gles2::GLES2Interface* gl_;
  gpu::ContextSupport* context_support_;
};

SkCanvas* MakeCommandBufferCanvas(int width, int height,
      gpu::gles2::GLES2Interface* gl, gpu::ContextSupport* context_support) {
  return new CommandBufferCanvas(width, height, gl, context_support);
}

} // namespace gpu
