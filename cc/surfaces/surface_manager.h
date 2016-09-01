// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_SURFACES_SURFACE_MANAGER_H_
#define CC_SURFACES_SURFACE_MANAGER_H_

#include <stdint.h>

#include <list>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "base/macros.h"
#include "base/observer_list.h"
#include "base/threading/thread_checker.h"
#include "cc/surfaces/surface_damage_observer.h"
#include "cc/surfaces/surface_id.h"
#include "cc/surfaces/surface_sequence.h"
#include "cc/surfaces/surfaces_export.h"

namespace gfx {
class Size;
}  // namespace gfx

namespace cc {
class BeginFrameSource;
class CompositorFrame;
class Surface;
class SurfaceFactoryClient;

class CC_SURFACES_EXPORT SurfaceManager {
 public:
  class Delegate {
   public:
    virtual ~Delegate() = default;
    virtual void OnSurfaceCreated(const gfx::Size& size,
                                  const SurfaceId& surface_id) = 0;
  };
  SurfaceManager(Delegate* delegate);
  ~SurfaceManager();

  void RegisterSurface(Surface* surface);
  void DeregisterSurface(const SurfaceId& surface_id);

  // Destroy the Surface once a set of sequence numbers has been satisfied.
  void Destroy(std::unique_ptr<Surface> surface);

  Surface* GetSurfaceForId(const SurfaceId& surface_id);

  void AddObserver(SurfaceDamageObserver* obs) {
    observer_list_.AddObserver(obs);
  }

  void RemoveObserver(SurfaceDamageObserver* obs) {
    observer_list_.RemoveObserver(obs);
  }

  bool SurfaceModified(const SurfaceId& surface_id);

  void DidCreateNewSurface(const gfx::Size& size, const SurfaceId& surface_id);

  // A frame for a surface satisfies a set of sequence numbers in a particular
  // id namespace.
  void DidSatisfySequences(uint32_t client_id, std::vector<uint32_t>* sequence);

  // void RegisterSurfaceClientId(uint32_t client_id);

  //// Invalidate a namespace that might still have associated sequences,
  //// possibly because a renderer process has crashed.
  // void InvalidateSurfaceClientId(uint32_t client_id);

  // SurfaceFactoryClient, hierarchy, and BeginFrameSource can be registered
  // and unregistered in any order with respect to each other.
  //
  // This happens in practice, e.g. the relationship to between ui::Compositor /
  // DelegatedFrameHost is known before ui::Compositor has a surface/client).
  // However, DelegatedFrameHost can register itself as a client before its
  // relationship with the ui::Compositor is known.

  // Associates a SurfaceFactoryClient with the surface id namespace it uses.
  // SurfaceFactoryClient and surface namespaces/allocators have a 1:1 mapping.
  // Caller guarantees the client is alive between register/unregister.
  // Reregistering the same namespace when a previous client is active is not
  // valid.
  void RegisterSurfaceFactoryClient(
      const CompositorFrameSinkId& compositor_frame_sink_id,
      SurfaceFactoryClient* client);
  void UnregisterSurfaceFactoryClient(
      const CompositorFrameSinkId& compositor_frame_sink_id);

  // Associates a |source| with a particular namespace.  That namespace and
  // any children of that namespace with valid clients can potentially use
  // that |source|.
  void RegisterBeginFrameSource(BeginFrameSource* source,
                                const CompositorFrameSinkId& sink_id);
  void UnregisterBeginFrameSource(BeginFrameSource* source);

  // Register a relationship between two namespaces.  This relationship means
  // that surfaces from the child namespace will be displayed in the parent.
  // Children are allowed to use any begin frame source that their parent can
  // use.
  void RegisterSurfaceNamespaceHierarchy(
      const CompositorFrameSinkId& parent_compositor_frame_sink_id,
      const CompositorFrameSinkId& child_compositor_frame_sink_id);

  void UnregisterSurfaceNamespaceHierarchy(
      const CompositorFrameSinkId& parent_compositor_frame_sink_id,
      const CompositorFrameSinkId& child_compositor_frame_sink_id);

  void AddRefOnSurfaceId(const SurfaceId& id);
  void RemoveRefOnSurfaceId(const SurfaceId& id);

 private:
  void RecursivelyAttachBeginFrameSource(
      const CompositorFrameSinkId& compositor_frame_sink_id,
      BeginFrameSource* source);
  void RecursivelyDetachBeginFrameSource(
      const CompositorFrameSinkId& compositor_frame_sink_id,
      BeginFrameSource* source);
  // Returns true if |child_compositor_frame_sink| is or has
  // |search_compositor_frame_sink| as a child.
  bool ChildContains(
      const CompositorFrameSinkId& child_compositor_frame_sink,
      const CompositorFrameSinkId& search_compositor_frame_sink) const;

  void GarbageCollectSurfaces();

  Delegate* const delegate_;
  using SurfaceMap = std::unordered_map<SurfaceId, Surface*, SurfaceIdHash>;
  SurfaceMap surface_map_;
  base::ObserverList<SurfaceDamageObserver> observer_list_;
  base::ThreadChecker thread_checker_;

  // List of surfaces to be destroyed, along with what sequences they're still
  // waiting on.
  using SurfaceDestroyList = std::list<std::unique_ptr<Surface>>;
  SurfaceDestroyList surfaces_to_destroy_;

  // Set of SurfaceSequences that have been satisfied by a frame but not yet
  // waited on.
  std::unordered_set<SurfaceSequence, SurfaceSequenceHash> satisfied_sequences_;

  // Begin frame source routing. Both BeginFrameSource and SurfaceFactoryClient
  // pointers guaranteed alive by callers until unregistered.
  struct ClientSourceMapping {
    ClientSourceMapping();
    ClientSourceMapping(const ClientSourceMapping& other);
    ~ClientSourceMapping();
    bool is_empty() const { return !client && children.empty(); }
    // The client that's responsible for creating this namespace.  Never null.
    SurfaceFactoryClient* client;
    // The currently assigned begin frame source for this client.
    BeginFrameSource* source;
    // This represents a dag of parent -> children mapping.
    std::unordered_set<CompositorFrameSinkId, CompositorFrameSinkIdHash>
        children;
  };
  std::unordered_map<CompositorFrameSinkId,
                     ClientSourceMapping,
                     CompositorFrameSinkIdHash>
      namespace_client_map_;
  // Set of which sources are registered to which namespace.  Any child
  // that is implicitly using this namespace must be reachable by the
  // parent in the dag.
  std::unordered_map<BeginFrameSource*, CompositorFrameSinkId>
      registered_sources_;

  struct SurfaceRefs {
    SurfaceRefs() = default;

    int refs = 0;
    int temp_refs = 0;  // Per renderer?
  };
  std::unordered_map<SurfaceId, SurfaceRefs, SurfaceIdHash> surface_refs_;

  DISALLOW_COPY_AND_ASSIGN(SurfaceManager);
};

}  // namespace cc

#endif  // CC_SURFACES_SURFACE_MANAGER_H_
