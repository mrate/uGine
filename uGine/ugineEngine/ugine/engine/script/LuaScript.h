#pragma once

#include <ugine/engine/core/Resource.h>
#include <ugine/engine/core/ResourceManager.h>

#include <ugine/String.h>

namespace ugine {

class LuaScript final : public Resource {
public:
    inline static const ResourceType TYPE{ "LuaScript" };

    LuaScript(ResourceManager& resourceManager, const ResourceID& id)
        : Resource{ resourceManager, TYPE, id }
        , source{ resourceManager.GetAllocator() } {}

    ~LuaScript() { Unload(); }

    String source;

private:
    bool HandleLoad(Span<const u8> data) override;
    bool HandleUnload() override;
};

} // namespace ugine