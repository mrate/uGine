#include <gfxapi/Device.h>
#include <gfxapi/Handle.h>

namespace ugine::gfxapi {
#define HANDLE_DELETER(HANDLE)                                                                                                                                 \
    template <> void HandleDeleter<HANDLE##Handle>::Delete(HANDLE##Handle handle) { m_device->Destroy##HANDLE(handle); }

HANDLE_DELETER(Buffer);
HANDLE_DELETER(Sampler);
HANDLE_DELETER(GraphicsPipeline);
HANDLE_DELETER(ComputePipeline);
HANDLE_DELETER(Texture);
HANDLE_DELETER(RenderPass);
HANDLE_DELETER(Framebuffer);
HANDLE_DELETER(Fence);
HANDLE_DELETER(Semaphore);
HANDLE_DELETER(Binding);
HANDLE_DELETER(QueryPool);

} // namespace ugine::gfxapi