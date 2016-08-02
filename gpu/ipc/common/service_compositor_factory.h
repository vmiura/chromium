#ifndef GPU_IPC_COMMON_SERVICE_COMPOSITOR_FACTORY_H_
#define GPU_IPC_COMMON_SERVICE_COMPOSITOR_FACTORY_H_

#include <memory>

namespace gpu {
class ServiceCompositor;

// See service_compositor.h for explanation of why this exists.
class ServiceCompositorFactory {
 public:
  virtual ~ServiceCompositorFactory() = default;

  virtual std::unique_ptr<ServiceCompositor> CreateServiceCompositor(
      int id) = 0;
};

}  // namespace gpu

#endif  // GPU_IPC_COMMON_SERVICE_COMPOSITOR_FACTORY_H_
