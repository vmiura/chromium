#include "cc/service/service_factory.h"

#include "base/memory/ptr_util.h"
#include "cc/service/service.h"

namespace cc {

ServiceFactory::ServiceFactory(
    SharedBitmapManager* shared_bitmap_manager,
    gpu::GpuMemoryBufferManager* gpu_memory_buffer_manager)
    : shared_bitmap_manager_(shared_bitmap_manager),
      gpu_memory_buffer_manager_(gpu_memory_buffer_manager) {}

ServiceFactory::~ServiceFactory() = default;

std::unique_ptr<gpu::ServiceCompositor> ServiceFactory::CreateServiceCompositor(
    int id) {
  return base::MakeUnique<Service>(id, shared_bitmap_manager_,
                                   gpu_memory_buffer_manager_,
                                   &task_graph_runner_);
}

}  // namespace cc
