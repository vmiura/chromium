#ifndef CC_SERVICE_SERVICE_FACTORY_H_
#define CC_SERVICE_SERVICE_FACTORY_H_

#include <memory>

#include "base/compiler_specific.h"
#include "base/threading/thread.h"
#include "cc/raster/single_thread_task_graph_runner.h"
#include "cc/service/compositor_channel.h"
#include "cc/service/service_export.h"
#include "cc/surfaces/surface_manager.h"

namespace gpu {
class GpuChannel;
class GpuMemoryBufferManager;
class ImageFactory;
}

namespace cc {
class SharedBitmapManager;
class SurfaceIdAllocator;
class SurfaceManager;

class CC_SERVICE_EXPORT ServiceFactory {
 public:
  ServiceFactory(SharedBitmapManager* shared_bitmap_manager,
                 gpu::GpuMemoryBufferManager* gpu_memory_buffer_manager,
                 gpu::ImageFactory* image_factory);
  ~ServiceFactory();

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
  SingleThreadTaskGraphRunner task_graph_runner_;
  base::ScopedPtrHashMap<int32_t, std::unique_ptr<CompositorChannel>>
      compositor_channels_;

  int next_service_id_ = 1;

  SurfaceManager surface_manager_;
  base::Thread compositor_thread_;
};

}  // namespace cc

#endif  // CC_SERVICE_SERVICE_FACTORY_H_
