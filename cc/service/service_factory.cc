#include "cc/service/service_factory.h"

#include "base/memory/ptr_util.h"
#include "base/threading/thread_task_runner_handle.h"
#include "cc/service/display_compositor.h"
#include "cc/service/service_context_provider.h"
#include "gpu/ipc/service/gpu_channel.h"

namespace cc {

namespace {

static void CreateDisplayCompositorOnCompositorThread(
    ServiceFactory* factory,
    std::unique_ptr<DisplayCompositor>* display_compositor,
    cc::mojom::DisplayCompositorRequest display_compositor_request,
    cc::mojom::DisplayCompositorClientPtrInfo display_compositor_client_info) {
  CHECK_EQ(nullptr, display_compositor->get());
  cc::mojom::DisplayCompositorClientPtr client_ptr;
  client_ptr.Bind(std::move(display_compositor_client_info));
  *display_compositor = base::WrapUnique(new cc::DisplayCompositor(
      factory, std::move(display_compositor_request), std::move(client_ptr),
      base::ThreadTaskRunnerHandle::Get()));
}

}  // namespace

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
  CHECK_EQ(nullptr, display_compositor_.get());
  // Create the display compositor on the compositor thread.
  compositor_thread_.task_runner()->PostTask(
      FROM_HERE,
      base::Bind(&CreateDisplayCompositorOnCompositorThread, this,
                 &display_compositor_, base::Passed(&display_compositor),
                 base::Passed(display_compositor_client.PassInterface())));
}

}  // namespace cc
