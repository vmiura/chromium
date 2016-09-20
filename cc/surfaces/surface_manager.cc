// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/surfaces/surface_manager.h"

#include <stddef.h>
#include <stdint.h>

#include "base/logging.h"
#include "cc/surfaces/surface.h"
#include "cc/surfaces/surface_factory_client.h"
#include "cc/surfaces/surface_id_allocator.h"

namespace cc {

SurfaceManager::ClientSourceMapping::ClientSourceMapping()
    : client(nullptr), source(nullptr) {}

SurfaceManager::ClientSourceMapping::ClientSourceMapping(
    const ClientSourceMapping& other) = default;

SurfaceManager::ClientSourceMapping::~ClientSourceMapping() {
  DCHECK(is_empty()) << "client: " << client
                     << ", children: " << children.size();
}

SurfaceManager::SurfaceManager(Delegate* delegate) : delegate_(delegate) {
  thread_checker_.DetachFromThread();
}

SurfaceManager::~SurfaceManager() {
  DCHECK(thread_checker_.CalledOnValidThread());
  for (SurfaceDestroyList::iterator it = surfaces_to_destroy_.begin();
       it != surfaces_to_destroy_.end();
       ++it) {
    DeregisterSurface((*it)->surface_id());
  }
  surfaces_to_destroy_.clear();

  // All hierarchies, sources, and surface factory clients should be
  // unregistered prior to SurfaceManager destruction.
  DCHECK_EQ(namespace_client_map_.size(), 0u);
  DCHECK_EQ(registered_sources_.size(), 0u);
}

void SurfaceManager::RegisterSurface(Surface* surface) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(surface);
  DCHECK(!surface_map_.count(surface->surface_id()));
  surface_map_[surface->surface_id()] = surface;
}

void SurfaceManager::DeregisterSurface(const SurfaceId& surface_id) {
  DCHECK(thread_checker_.CalledOnValidThread());
  SurfaceMap::iterator it = surface_map_.find(surface_id);
  DCHECK(it != surface_map_.end());
  surface_map_.erase(it);
}

void SurfaceManager::Destroy(std::unique_ptr<Surface> surface) {
  DCHECK(thread_checker_.CalledOnValidThread());
  surface->set_destroyed(true);
  surfaces_to_destroy_.push_back(std::move(surface));
  GarbageCollectSurfaces();
}

void SurfaceManager::GarbageCollectSurfaces() {
  std::vector<SurfaceId> live_surfaces;
  std::set<SurfaceId> live_surfaces_set;

  // GC roots are surfaces that have not been destroyed.
  for (auto& map_entry : surface_map_) {
    if (!map_entry.second->destroyed()) {
      live_surfaces_set.insert(map_entry.first);
      live_surfaces.push_back(map_entry.first);
    }
  }

  // Mark all surfaces reachable from live surfaces by adding them to
  // live_surfaces and live_surfaces_set.
  for (size_t i = 0; i < live_surfaces.size(); i++) {
    Surface* surf = surface_map_[live_surfaces[i]];
    DCHECK(surf);
    for (const SurfaceId& id : surf->referenced_surfaces()) {
      if (live_surfaces_set.count(id))
        continue;
      Surface* surf2 = GetSurfaceForId(id);
      if (surf2) {
        live_surfaces.push_back(id);
        live_surfaces_set.insert(id);
      }
    }
  }

  std::vector<std::unique_ptr<Surface>> to_destroy;

  // Destroy all remaining unreachable surfaces.
  for (SurfaceDestroyList::iterator dest_it = surfaces_to_destroy_.begin();
       dest_it != surfaces_to_destroy_.end();) {
    auto& refs = surface_refs_[(*dest_it)->surface_id()];
    if (refs.refs) {
      ++dest_it;
      continue;
    }

    if (!live_surfaces_set.count((*dest_it)->surface_id())) {
      std::unique_ptr<Surface> surf(std::move(*dest_it));
      LOG(ERROR) << "** DELETING SurfaceId " << surf->surface_id().ToString();
      DeregisterSurface(surf->surface_id());
      dest_it = surfaces_to_destroy_.erase(dest_it);
      to_destroy.push_back(std::move(surf));
    } else {
      ++dest_it;
    }
  }

  to_destroy.clear();
}

void SurfaceManager::RegisterSurfaceFactoryClient(
    const CompositorFrameSinkId& compositor_frame_sink_id,
    SurfaceFactoryClient* client) {
  DCHECK(client);
  DCHECK(!namespace_client_map_[compositor_frame_sink_id].client);

  auto iter = namespace_client_map_.find(compositor_frame_sink_id);
  if (iter == namespace_client_map_.end()) {
    auto insert_result = namespace_client_map_.insert(
        std::make_pair(compositor_frame_sink_id, ClientSourceMapping()));
    DCHECK(insert_result.second);
    iter = insert_result.first;
  }
  iter->second.client = client;

  // Propagate any previously set sources to the new client.
  if (iter->second.source)
    client->SetBeginFrameSource(iter->second.source);
}

void SurfaceManager::UnregisterSurfaceFactoryClient(
    const CompositorFrameSinkId& compositor_frame_sink_id) {
  DCHECK_EQ(namespace_client_map_.count(compositor_frame_sink_id), 1u);

  auto iter = namespace_client_map_.find(compositor_frame_sink_id);
  if (iter->second.source)
    iter->second.client->SetBeginFrameSource(nullptr);
  iter->second.client = nullptr;

  // The SurfaceFactoryClient and hierarchy can be registered/unregistered
  // in either order, so empty namespace_client_map entries need to be
  // checked when removing either clients or relationships.
  if (iter->second.is_empty())
    namespace_client_map_.erase(iter);
}

void SurfaceManager::RegisterBeginFrameSource(
    BeginFrameSource* source,
    const CompositorFrameSinkId& sink_id) {
  DCHECK(source);
  DCHECK_EQ(registered_sources_.count(source), 0u);

  registered_sources_[source] = sink_id;
  RecursivelyAttachBeginFrameSource(sink_id, source);
}

void SurfaceManager::UnregisterBeginFrameSource(BeginFrameSource* source) {
  DCHECK(source);
  DCHECK_EQ(registered_sources_.count(source), 1u);

  CompositorFrameSinkId compositor_frame_sink_id = registered_sources_[source];
  registered_sources_.erase(source);

  if (namespace_client_map_.count(compositor_frame_sink_id) == 0u)
    return;

  // TODO(enne): these walks could be done in one step.
  // Remove this begin frame source from its subtree.
  RecursivelyDetachBeginFrameSource(compositor_frame_sink_id, source);
  // Then flush every remaining registered source to fix any sources that
  // became null because of the previous step but that have an alternative.
  for (auto source_iter : registered_sources_)
    RecursivelyAttachBeginFrameSource(source_iter.second, source_iter.first);
}

void SurfaceManager::RecursivelyAttachBeginFrameSource(
    const CompositorFrameSinkId& compositor_frame_sink_id,
    BeginFrameSource* source) {
  ClientSourceMapping& mapping =
      namespace_client_map_[compositor_frame_sink_id];
  if (!mapping.source) {
    mapping.source = source;
    if (mapping.client)
      mapping.client->SetBeginFrameSource(source);
  }
  for (const CompositorFrameSinkId& child_compositor_frame_sink_id :
       mapping.children)
    RecursivelyAttachBeginFrameSource(child_compositor_frame_sink_id, source);
}

void SurfaceManager::RecursivelyDetachBeginFrameSource(
    const CompositorFrameSinkId& compositor_frame_sink_id,
    BeginFrameSource* source) {
  auto iter = namespace_client_map_.find(compositor_frame_sink_id);
  if (iter == namespace_client_map_.end())
    return;
  if (iter->second.source == source) {
    iter->second.source = nullptr;
    if (iter->second.client)
      iter->second.client->SetBeginFrameSource(nullptr);
  }

  if (iter->second.is_empty()) {
    namespace_client_map_.erase(iter);
    return;
  }

  for (const CompositorFrameSinkId& child_sink_id : iter->second.children) {
    RecursivelyDetachBeginFrameSource(child_sink_id, source);
  }
}

bool SurfaceManager::ChildContains(
    const CompositorFrameSinkId& child_compositor_frame_sink_id,
    const CompositorFrameSinkId& search_compositor_frame_sink_id) const {
  auto iter = namespace_client_map_.find(child_compositor_frame_sink_id);
  if (iter == namespace_client_map_.end())
    return false;

  for (const CompositorFrameSinkId& child_sink_id : iter->second.children) {
    if (child_sink_id == search_compositor_frame_sink_id)
      return true;
    if (ChildContains(child_sink_id, search_compositor_frame_sink_id))
      return true;
  }
  return false;
}

void SurfaceManager::RegisterSurfaceNamespaceHierarchy(
    const CompositorFrameSinkId& parent_compositor_frame_sink_id,
    const CompositorFrameSinkId& child_compositor_frame_sink_id) {
  // If it's possible to reach the parent through the child's descendant chain,
  // then this will create an infinite loop.  Might as well just crash here.
  CHECK(!ChildContains(child_compositor_frame_sink_id,
                       parent_compositor_frame_sink_id));

  auto& children =
      namespace_client_map_[parent_compositor_frame_sink_id].children;
  // If the child is already registered because we throw away a FrameSink and
  // recreate it again, then this is OK.
  // DCHECK_EQ(0u, children.count(child_compositor_frame_sink_id));
  children.insert(child_compositor_frame_sink_id);

  // If the parent has no source, then attaching it to this child will
  // not change any downstream sources.
  BeginFrameSource* parent_source =
      namespace_client_map_[parent_compositor_frame_sink_id].source;
  if (!parent_source)
    return;

  DCHECK_EQ(registered_sources_.count(parent_source), 1u);
  RecursivelyAttachBeginFrameSource(child_compositor_frame_sink_id,
                                    parent_source);
}

void SurfaceManager::UnregisterSurfaceNamespaceHierarchy(
    const CompositorFrameSinkId& parent_compositor_frame_sink_id,
    const CompositorFrameSinkId& child_compositor_frame_sink_id) {
  // Deliberately do not check validity of either parent or child namespace
  // here.  They were valid during the registration, so were valid at some
  // point in time.  This makes it possible to invalidate parent and child
  // namespaces independently of each other and not have an ordering dependency
  // of unregistering the hierarchy first before either of them.
  DCHECK_EQ(namespace_client_map_.count(parent_compositor_frame_sink_id), 1u);

  auto iter = namespace_client_map_.find(parent_compositor_frame_sink_id);

  auto& children = iter->second.children;
  auto it = children.find(child_compositor_frame_sink_id);
  bool found_child = it != children.end();
  children.erase(it);
  DCHECK(found_child);

  // The SurfaceFactoryClient and hierarchy can be registered/unregistered
  // in either order, so empty namespace_client_map entries need to be
  // checked when removing either clients or relationships.
  if (iter->second.is_empty()) {
    namespace_client_map_.erase(iter);
    return;
  }

  // If the parent does not have a begin frame source, then disconnecting it
  // will not change any of its children.
  BeginFrameSource* parent_source = iter->second.source;
  if (!parent_source)
    return;

  // TODO(enne): these walks could be done in one step.
  RecursivelyDetachBeginFrameSource(child_compositor_frame_sink_id,
                                    parent_source);
  for (auto source_iter : registered_sources_)
    RecursivelyAttachBeginFrameSource(source_iter.second, source_iter.first);
}

Surface* SurfaceManager::GetSurfaceForId(const SurfaceId& surface_id) {
  DCHECK(thread_checker_.CalledOnValidThread());
  SurfaceMap::iterator it = surface_map_.find(surface_id);
  if (it == surface_map_.end())
    return NULL;
  return it->second;
}

bool SurfaceManager::SurfaceModified(const SurfaceId& surface_id) {
  CHECK(thread_checker_.CalledOnValidThread());
  bool changed = false;
  FOR_EACH_OBSERVER(SurfaceDamageObserver, observer_list_,
                    OnSurfaceDamaged(surface_id, &changed));
  return changed;
}

void SurfaceManager::DidCreateNewSurface(const gfx::Size& size,
                                         const SurfaceId& surface_id) {
  if (delegate_)
    delegate_->OnSurfaceCreated(size, surface_id);
}

void SurfaceManager::AddRefOnSurfaceId(const SurfaceId& id) {
  if (id.is_null()) return;
  auto& refs = surface_refs_[id];
  refs.refs++;
  LOG(ERROR) << "Add ref on SurfaceId " << id.ToString() << " " << refs.refs;
}

void SurfaceManager::RemoveRefOnSurfaceId(const SurfaceId& id) {
  if (id.is_null()) return;
  auto it = surface_refs_.find(id);
  // DCHECK(it != surface_refs_.end());
  // TODO(fsamuel): We may try to reap surfaces from an old instance of the
  // display compositor. This is OK but icky. Because SurfaceIds have a nonce,
  // it is highly unlikely for a surface ID in the old display compositor to
  // match a surface ID generated for the new display compositor for now (since
  // the display compositor is generating the surface ID).
  if (it == surface_refs_.end())
    return;
  auto& refs = it->second;
  DCHECK_GT(refs.refs, 0);
  refs.refs--;
  // If this SurfaceId has no refs then we can garbage collect it.
  if (!refs.refs)
    GarbageCollectSurfaces();
  LOG(ERROR) << "Remove ref on SurfaceId " << id.ToString() << " " << refs.refs;
}

}  // namespace cc
