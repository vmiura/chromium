// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SVGShapePainter_h
#define SVGShapePainter_h

#include "third_party/skia/include/core/SkPath.h"
#include "skia/ext/cdl_common.h"
#include "wtf/Allocator.h"

namespace blink {

class FloatRect;
class GraphicsContext;
class LayoutSVGResourceMarker;
class LayoutSVGShape;
struct MarkerPosition;
struct PaintInfo;

class SVGShapePainter {
  STACK_ALLOCATED();

 public:
  SVGShapePainter(const LayoutSVGShape& layoutSVGShape)
      : m_layoutSVGShape(layoutSVGShape) {}

  void paint(const PaintInfo&);

 private:
  void fillShape(GraphicsContext&, const CdlPaint&, SkPath::FillType);
  void strokeShape(GraphicsContext&, const CdlPaint&);

  void paintMarkers(const PaintInfo&, const FloatRect& boundingBox);
  void paintMarker(const PaintInfo&,
                   const LayoutSVGResourceMarker&,
                   const MarkerPosition&,
                   float);

  const LayoutSVGShape& m_layoutSVGShape;
};

}  // namespace blink

#endif  // SVGShapePainter_h
