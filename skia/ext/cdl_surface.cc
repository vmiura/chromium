/*
 * Copyright 2016 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "cdl_common.h"

#if CDL_ENABLED

#include "cdl_surface.h"

#include "base/memory/ptr_util.h"
#include "cdl_canvas.h"

CdlSurface::CdlSurface(sk_sp<SkSurface> surface)
    : surface_(std::move(surface)) {}
CdlSurface::~CdlSurface() {}

sk_sp<SkImage> CdlSurface::makeImageSnapshot(SkBudgeted budgeted) {
  return surface_->makeImageSnapshot(budgeted);
}

sk_sp<CdlSurface> CdlSurface::MakeRasterDirect(const SkImageInfo& info,
                                               void* pixels,
                                               size_t rowBytes,
                                               const SkSurfaceProps* props) {
  return sk_make_sp<CdlSurface>(
      SkSurface::MakeRasterDirect(info, pixels, rowBytes, props));
}

/**
*  The same as NewRasterDirect, but also accepts a call-back routine, which is
* invoked
*  when the surface is deleted, and is passed the pixel memory and the specified
* context.
*/
sk_sp<CdlSurface> CdlSurface::MakeRasterDirectReleaseProc(
    const SkImageInfo& info,
    void* pixels,
    size_t rowBytes,
    void (*releaseProc)(void* pixels, void* context),
    void* context,
    const SkSurfaceProps* props) {
  return sk_make_sp<CdlSurface>(SkSurface::MakeRasterDirectReleaseProc(
      info, pixels, rowBytes, releaseProc, context, props));
}

/**
*  Return a new surface, with the memory for the pixels automatically allocated
* and
*  zero-initialized, but respecting the specified rowBytes. If rowBytes==0, then
* a default
*  value will be chosen. If a non-zero rowBytes is specified, then any images
* snapped off of
*  this surface (via makeImageSnapshot()) are guaranteed to have the same
* rowBytes.
*
*  If the requested surface cannot be created, or the request is not a
*  supported configuration, NULL will be returned.
*/
sk_sp<CdlSurface> CdlSurface::MakeRaster(const SkImageInfo& info,
                                         size_t rowBytes,
                                         const SkSurfaceProps* props) {
  return sk_make_sp<CdlSurface>(SkSurface::MakeRaster(info, rowBytes, props));
}

#if 0
/**
*  Used to wrap a pre-existing backend 3D API texture as a SkSurface. The kRenderTarget flag
*  must be set on GrBackendTextureDesc for this to succeed. Skia will not assume ownership
*  of the texture and the client must ensure the texture is valid for the lifetime of the
*  SkSurface.
*/
static sk_sp<CdlSurface> CdlSurface::MakeFromBackendTexture(GrContext* context, const GrBackendTextureDesc& desc,
                                           sk_sp<SkColorSpace> color, const SkSurfaceProps* props) {
  return sk_make_sp<CdlSurface>(SkSurface::MakeFromBackendTexture(context, desc, color, props));
}

/**
*  Used to wrap a pre-existing 3D API rendering target as a SkSurface. Skia will not assume
*  ownership of the render target and the client must ensure the render target is valid for the
*  lifetime of the SkSurface.
*/
static sk_sp<CdlSurface> CdlSurface::MakeFromBackendRenderTarget(GrContext*,
                                                const GrBackendRenderTargetDesc&,
                                                sk_sp<SkColorSpace>,
                                                const SkSurfaceProps*) {

}

/**
*  Used to wrap a pre-existing 3D API texture as a SkSurface. Skia will treat the texture as
*  a rendering target only, but unlike NewFromBackendRenderTarget, Skia will manage and own
*  the associated render target objects (but not the provided texture). The kRenderTarget flag
*  must be set on GrBackendTextureDesc for this to succeed. Skia will not assume ownership
*  of the texture and the client must ensure the texture is valid for the lifetime of the
*  SkSurface.
*/
static sk_sp<CdlSurface> CdlSurface::MakeFromBackendTextureAsRenderTarget(
GrContext*, const GrBackendTextureDesc&, sk_sp<SkColorSpace>, const SkSurfaceProps*) {

}
#endif

/**
*  Return a new surface whose contents will be drawn to an offscreen
*  render target, allocated by the surface.
*/
sk_sp<CdlSurface> CdlSurface::MakeRenderTarget(GrContext* gc,
                                               SkBudgeted budgeted,
                                               const SkImageInfo& info,
                                               int sampleCount,
                                               GrSurfaceOrigin origin,
                                               const SkSurfaceProps* props) {
  return sk_make_sp<CdlSurface>(SkSurface::MakeRenderTarget(
      gc, budgeted, info, sampleCount, origin, props));
}

CdlCanvas* CdlSurface::getCanvas() {
  if (canvas_.get())
    return canvas_.get();
  canvas_ = base::MakeUnique<CdlPassThroughCanvas>(surface_->getCanvas());
  return canvas_.get();
}

void CdlSurface::draw(CdlCanvas* canvas,
                      SkScalar x,
                      SkScalar y,
                      const CdlPaint* paint) {
  auto image = this->makeImageSnapshot(SkBudgeted::kYes);
  if (image) {
    canvas->drawImage(image, x, y, paint);
  }
}

#endif  // CDL_ENABLED
