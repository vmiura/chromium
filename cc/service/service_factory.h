#ifndef CC_SERVICE_SERVICE_FACTORY_H_
#define CC_SERVICE_SERVICE_FACTORY_H_

#include "base/compiler_specific.h"
#include "cc/raster/single_thread_task_graph_runner.h"
#include "cc/service/compositor_channel.h"
#include "cc/service/service_export.h"

namespace gpu {
class GpuChannel;
class GpuMemoryBufferManager;
}

namespace cc {
class SharedBitmapManager;

class CC_SERVICE_EXPORT ServiceFactory {
 public:
  ServiceFactory(SharedBitmapManager* shared_bitmap_manager,
                 gpu::GpuMemoryBufferManager* gpu_memory_buffer_manager);
  ~ServiceFactory();

  void AddChannel(gpu::GpuChannel* channel);
  void RemoveChannel(int32_t client_id);
  void DestroyAllChannels();

  SharedBitmapManager* shared_bitmap_manager() {
    return shared_bitmap_manager_;
  }

  gpu::GpuMemoryBufferManager* gpu_memory_buffer_manager() {
    return gpu_memory_buffer_manager_;
  }

  SingleThreadTaskGraphRunner* task_graph_runner() {
    return &task_graph_runner_;
  }

 private:
  SharedBitmapManager* shared_bitmap_manager_;
  gpu::GpuMemoryBufferManager* gpu_memory_buffer_manager_;
  SingleThreadTaskGraphRunner task_graph_runner_;
  base::ScopedPtrHashMap<int32_t, std::unique_ptr<CompositorChannel>>
      compositor_channels_;
};

}  // namespace cc

#endif  // CC_SERVICE_SERVICE_FACTORY_H_
