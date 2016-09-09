// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_LAYERS_SURFACE_LAYER_H_
#define CC_LAYERS_SURFACE_LAYER_H_

#include "base/macros.h"
#include "cc/base/cc_export.h"
#include "cc/layers/layer.h"
#include "cc/surfaces/surface_id.h"
#include "ui/gfx/geometry/size.h"

namespace cc {

// A layer that renders a surface referencing the output of another compositor
// instance or client.
class CC_EXPORT SurfaceLayer : public Layer {
 public:
  using AddRefCallback = base::Callback<void(const SurfaceId&)>;
  using ReleaseRefCallback = base::Callback<void(const SurfaceId&)>;

  static scoped_refptr<SurfaceLayer> Create(
      const AddRefCallback& addref_callback,
      const ReleaseRefCallback& release_callback);

  void WriteStructureMojom(const ContentFrameBuilderContext& context,
                           cc::mojom::LayerStructure* mojom) override;
  void WritePropertiesMojom(const ContentFrameBuilderContext& context,
                            cc::mojom::LayerProperties* mojom) override;

  void SetSurfaceId(const SurfaceId& surface_id,
                    float scale,
                    const gfx::Size& size);

  // Layer overrides.
  std::unique_ptr<LayerImpl> CreateLayerImpl(LayerTreeImpl* tree_impl) override;
  void SetLayerTreeHost(LayerTreeHost* host) override;
  void PushPropertiesTo(LayerImpl* layer) override;

 protected:
  SurfaceLayer(const AddRefCallback& addref_callback,
               const ReleaseRefCallback& release_callback);
  bool HasDrawableContent() const override;

 private:
  ~SurfaceLayer() override;

  SurfaceId surface_id_;
  gfx::Size surface_size_;
  float surface_scale_;
  AddRefCallback addref_callback_;
  ReleaseRefCallback release_callback_;

  DISALLOW_COPY_AND_ASSIGN(SurfaceLayer);
};

}  // namespace cc

#endif  // CC_LAYERS_SURFACE_LAYER_H_
