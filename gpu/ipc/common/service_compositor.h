#ifndef GPU_IPC_COMMON_SERVICE_COMPOSITOR_H_
#define GPU_IPC_COMMON_SERVICE_COMPOSITOR_H_

#include <memory>

namespace gpu {

// This virtual indrection exists to avoid including or depending on cc
// from in gpu/ipc/service because:
// - Circular GN dependecies: cc -> gpu -> service -> cc
// - cc uses client GL headers, but gpu/ipc/service uses real GL headers, so
// symbols collide if anything from cc that includes a GL header gets
// transitively included from gpu/ipc/service.
class ServiceCompositor {
 public:
  virtual ~ServiceCompositor() = default;
};

}  // namespace gpu

#endif  // GPU_IPC_COMMON_SERVICE_COMPOSITOR_H_
