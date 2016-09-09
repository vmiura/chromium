// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/graphics/CanvasSurfaceLayerBridge.h"

#include "cc/layers/surface_layer.h"
#include "cc/surfaces/surface_id.h"
#include "platform/graphics/GraphicsLayer.h"
#include "platform/mojo/MojoHelper.h"
#include "public/platform/Platform.h"
#include "public/platform/WebCompositorSupport.h"
#include "public/platform/WebLayer.h"
#include "ui/gfx/geometry/size.h"
#include "wtf/Functional.h"
#include "wtf/PtrUtil.h"

namespace blink {

CanvasSurfaceLayerBridge::CanvasSurfaceLayerBridge(std::unique_ptr<CanvasSurfaceLayerBridgeClient> client)
{
    m_client = std::move(client);
}

CanvasSurfaceLayerBridge::~CanvasSurfaceLayerBridge()
{
}

bool CanvasSurfaceLayerBridge::createSurfaceLayer(int canvasWidth, int canvasHeight)
{
    if (!m_client->syncGetSurfaceId(&m_surfaceId))
        return false;

    m_client->asyncRequestSurfaceCreation(m_surfaceId);
    //m_surfaceLayer = cc::SurfaceLayer::Create(std::move(satisfyCallback), std::move(requireCallback));
    //m_surfaceLayer->SetSurfaceId(m_surfaceId, 1.f, gfx::Size(canvasWidth, canvasHeight));

    //m_webLayer = wrapUnique(Platform::current()->compositorSupport()->createLayerFromCCLayer(m_surfaceLayer.get()));
    //GraphicsLayer::registerContentsLayer(m_webLayer.get());
    return true;
}

} // namespace blink
