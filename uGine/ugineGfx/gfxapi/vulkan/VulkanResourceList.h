#pragma once

#include <gfxapi/Handle.h>

#include <cassert>
#include <vector>

namespace ugine::gfxapi {

struct ResourceList {
    std::vector<BufferHandle> buffers;
    std::vector<SamplerHandle> samplers;
    std::vector<GraphicsPipelineHandle> graphicsPipelines;
    std::vector<ComputePipelineHandle> computePipelines;
    std::vector<TextureHandle> textures;
    std::vector<RenderPassHandle> renderPasses;
    std::vector<FramebufferHandle> framebuffers;
    std::vector<FenceHandle> fences;
    std::vector<SemaphoreHandle> semaphores;
    std::vector<vk::UniqueDescriptorPool> descriptorPools;
    std::vector<BindingHandle> bindings;
    std::vector<QueryPoolHandle> queryPools;

    void Clear() {
        buffers.clear();
        samplers.clear();
        graphicsPipelines.clear();
        computePipelines.clear();
        textures.clear();
        renderPasses.clear();
        framebuffers.clear();
        fences.clear();
        semaphores.clear();
        descriptorPools.clear();
        bindings.clear();
        queryPools.clear();

        assert(Empty());
    }

    bool Empty() const {
        return buffers.empty() &&        //
            samplers.empty() &&          //
            graphicsPipelines.empty() && //
            computePipelines.empty() &&  //
            textures.empty() &&          //
            renderPasses.empty() &&      //
            framebuffers.empty() &&      //
            fences.empty() &&            //
            semaphores.empty() &&        //
            descriptorPools.empty() &&   //
            bindings.empty() &&          //
            queryPools.empty();
    }
};

} // namespace ugine::gfxapi