#pragma once

#include <ugine/Ugine.h>

#include <gfxapi/CommandList.h>

namespace ugine {

struct GpuQuery {
    u32 begin{};
    u32 end{};
};

struct GpuScopeQuery {
    GpuScopeQuery(gfxapi::CommandList& cmd, gfxapi::QueryPoolHandle pool, GpuQuery& query)
        : cmd_{ cmd }
        , pool_{ pool }
        , query_{ query } {
        query_.begin = cmd_.WriteTimestamp(pool_, gfxapi::PipelineStage::TopOfPipeline);
    }

    ~GpuScopeQuery() { query_.end = cmd_.WriteTimestamp(pool_, gfxapi::PipelineStage::BottomOfPipeline); }

private:
    gfxapi::CommandList& cmd_;
    gfxapi::QueryPoolHandle pool_;
    GpuQuery& query_;
};

} // namespace ugine