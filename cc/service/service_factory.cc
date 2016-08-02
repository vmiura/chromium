#include "cc/service/service_factory.h"

#include "base/memory/ptr_util.h"
#include "cc/service/service.h"
#include "gpu/ipc/service/gpu_channel.h"

namespace cc {

ServiceFactory::ServiceFactory(
    SharedBitmapManager* shared_bitmap_manager,
    gpu::GpuMemoryBufferManager* gpu_memory_buffer_manager)
    : shared_bitmap_manager_(shared_bitmap_manager),
      gpu_memory_buffer_manager_(gpu_memory_buffer_manager) {}

ServiceFactory::~ServiceFactory() = default;

void ServiceFactory::AddChannel(gpu::GpuChannel* channel) {
  std::unique_ptr<CompositorChannel> compositor_channel(
      new CompositorChannel(this, channel));
  compositor_channels_.set(channel->client_id(), std::move(compositor_channel));
}

void ServiceFactory::RemoveChannel(int32_t client_id) {
  compositor_channels_.erase(client_id);
}

void ServiceFactory::DestroyAllChannels() {
  compositor_channels_.clear();
}

}  // namespace cc
