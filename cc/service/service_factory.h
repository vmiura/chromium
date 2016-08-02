#ifndef CC_SERVICE_SERVICE_FACTORY_H_
#define CC_SERVICE_SERVICE_FACTORY_H_

#include "base/compiler_specific.h"
#include "cc/raster/single_thread_task_graph_runner.h"
#include "cc/service/service_export.h"
#include "gpu/ipc/common/service_compositor_factory.h"

namespace gpu {
class GpuMemoryBufferManager;
class ServiceCompositor;
}

namespace cc {
class SharedBitmapManager;

class CC_SERVICE_EXPORT ServiceFactory
    : NON_EXPORTED_BASE(public gpu::ServiceCompositorFactory) {
 public:
  ServiceFactory(SharedBitmapManager* shared_bitmap_manager,
                 gpu::GpuMemoryBufferManager* gpu_memory_buffer_manager);
  ~ServiceFactory() override;

  std::unique_ptr<gpu::ServiceCompositor> CreateServiceCompositor(
      int id) override;

 private:
  SharedBitmapManager* shared_bitmap_manager_;
  gpu::GpuMemoryBufferManager* gpu_memory_buffer_manager_;
  SingleThreadTaskGraphRunner task_graph_runner_;
};

}  // namespace cc

#endif  // CC_SERVICE_SERVICE_FACTORY_H_
