#include "cc/service/service_factory.h"

#include "base/memory/ptr_util.h"
#include "cc/service/display_compositor.h"
#include "cc/service/service.h"
#include "cc/service/service_context_provider.h"
#include "gpu/ipc/service/gpu_channel.h"

namespace cc {

ServiceFactory::ServiceFactory(
    SharedBitmapManager* shared_bitmap_manager,
    gpu::GpuMemoryBufferManager* gpu_memory_buffer_manager,
    gpu::ImageFactory* image_factory,
    gpu::SyncPointManager* sync_point_manager,
    gpu::gles2::MailboxManager* mailbox_manager)
    : shared_bitmap_manager_(shared_bitmap_manager),
      gpu_memory_buffer_manager_(gpu_memory_buffer_manager),
      image_factory_(image_factory),
      compositor_thread_("compositor"),
      weak_factory_(this) {
  ServiceContextProvider::SetupThread(sync_point_manager, mailbox_manager);
  task_graph_runner_.Start("CompositorWorker", base::SimpleThread::Options());
  compositor_thread_.Start();
}

ServiceFactory::~ServiceFactory() = default;

void ServiceFactory::BindDisplayCompositorFactoryRequest(
    cc::mojom::DisplayCompositorFactoryRequest request) {
  bindings_.AddBinding(this, std::move(request));
}

void ServiceFactory::CreateDisplayCompositor(
    cc::mojom::DisplayCompositorRequest display_compositor,
    cc::mojom::DisplayCompositorClientPtr display_compositor_client) {
  display_compositors_.insert(base::WrapUnique(new cc::DisplayCompositor(
      this, std::move(display_compositor), std::move(display_compositor_client),
      compositor_thread_.task_runner())));
}

void ServiceFactory::AddChannel(gpu::GpuChannel* channel) {
  channel->AddAssociatedInterface(
      base::Bind(&ServiceFactory::AddChannelInternal,
                 weak_factory_.GetWeakPtr(), channel->client_id()));
}

void ServiceFactory::RemoveChannel(int32_t client_id) {
  compositor_channels_.erase(client_id);
}

void ServiceFactory::DestroyAllChannels() {
  compositor_channels_.clear();
}

void ServiceFactory::AddChannelInternal(
    int32_t client_id,
    cc::mojom::CompositorChannelAssociatedRequest request) {
  std::unique_ptr<CompositorChannel> compositor_channel(new CompositorChannel(
      std::move(request), this, compositor_thread_.task_runner()));
  compositor_channels_.set(client_id, std::move(compositor_channel));
}

}  // namespace cc
