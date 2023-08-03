#pragma once

#include <ugine/Error.h>

#include <vulkan/vulkan.hpp>

#include <optional>

namespace ugine::gfxapi {

class GfxError : public Error {
public:
    GfxError(const char* msg, const char* name, const char* file, int line, const char* function, std::optional<vk::Result> result = {}) noexcept
        : ugine::Error(msg, name, file, line, function)
        , result_(result) {}

    std::optional<vk::Result> Result() const noexcept { return result_; }

private:
    std::optional<vk::Result> result_;
};

} // namespace ugine::gfxapi
