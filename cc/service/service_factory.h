#ifndef CC_SERVICE_SERVICE_FACTORY_H_
#define CC_SERVICE_SERVICE_FACTORY_H_

#include <memory>

#include "base/compiler_specific.h"
#include "base/threading/thread.h"
#include "cc/ipc/compositor.mojom.h"
#include "cc/raster/single_thread_task_graph_runner.h"
#include "cc/service/compositor_channel.h"
#include "cc/service/service_export.h"
#include "cc/surfaces/surface_manager.h"
#include "mojo/public/cpp/bindings/binding_set.h"

namespace gpu {
class GpuChannel;
class GpuMemoryBufferManager;
class ImageFactory;
class SyncPointManager;
namespace gles2 {
class MailboxManager;
}
}

namespace cc {
class DisplayCompositor;
class SharedBitmapManager;
class SurfaceIdAllocator;
class SurfaceManager;

class CC_SERVICE_EXPORT ServiceFactory
    : public cc::mojom::DisplayCompositorFactory {
 public:
  ServiceFactory(SharedBitmapManager* shared_bitmap_manager,
                 gpu::GpuMemoryBufferManager* gpu_memory_buffer_manager,
                 gpu::ImageFactory* image_factory,
                 gpu::SyncPointManager* sync_point_manager,
                 gpu::gles2::MailboxManager* mailbox_manager);
  ~ServiceFactory() override;

  void BindDisplayCompositorFactoryRequest(
      cc::mojom::DisplayCompositorFactoryRequest request);

  // cc::mojom::DisplayCompositorFactory implementation.
  void CreateDisplayCompositor(
      cc::mojom::DisplayCompositorRequest display_compositor,
      cc::mojom::DisplayCompositorClientPtr display_compositor_client) override;

  void AddChannel(gpu::GpuChannel* channel);
  void RemoveChannel(int32_t client_id);
  void DestroyAllChannels();

  // Accessors for CompositorChannel to use for creating a Service.
  SharedBitmapManager* shared_bitmap_manager() {
    return shared_bitmap_manager_;
  }
  gpu::GpuMemoryBufferManager* gpu_memory_buffer_manager() {
    return gpu_memory_buffer_manager_;
  }
  gpu::ImageFactory* image_factory() { return image_factory_; }
  SingleThreadTaskGraphRunner* task_graph_runner() {
    return &task_graph_runner_;
  }
  SurfaceManager* surface_manager() { return &surface_manager_; }
  int NextServiceCompositorId() { return next_service_id_++; }

 private:
  SharedBitmapManager* shared_bitmap_manager_;
  gpu::GpuMemoryBufferManager* gpu_memory_buffer_manager_;
  gpu::ImageFactory* image_factory_;
  base::ScopedPtrHashMap<int32_t, std::unique_ptr<CompositorChannel>>
      compositor_channels_;

  int next_service_id_ = 1;

  // Used on the compositor thread.
  SingleThreadTaskGraphRunner task_graph_runner_;
  SurfaceManager surface_manager_;

  base::Thread compositor_thread_;

  std::set<std::unique_ptr<DisplayCompositor>> display_compositors_;
  // Bindings to cc::mojom::DisplayCompositorFactory.
  mojo::BindingSet<cc::mojom::DisplayCompositorFactory> bindings_;
};

}  // namespace cc

#endif  // CC_SERVICE_SERVICE_FACTORY_H_
