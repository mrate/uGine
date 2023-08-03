#pragma once

#include <stdint.h>

namespace ugine {

// Gfx limits;
static constexpr u32 MAX_COMMANDLIST_COUNT{ 32 };
static constexpr auto MAX_SECONDARY_BUFFERS{ 128 };
static constexpr auto MAX_PUSHCONSTANT_SIZE{ 128 };
static constexpr auto MAX_COLOR_ATTACHMENTS{ 4 };
static constexpr int MAX_FRAMES_IN_FLIGHT{ 3 };

// Rendering limits;

static constexpr int CSM_LEVELS{ 3 };

static constexpr auto MAX_LIGHTS{ 8192 };

static constexpr auto MAX_DIR_SHADOWS{ 1 };
static constexpr auto MAX_SPOT_SHADOWS{ 8 };
static constexpr auto MAX_POINT_SHADOWS{ 8 };

static constexpr auto MAX_SHADOWS{ MAX_DIR_SHADOWS + MAX_SPOT_SHADOWS + MAX_POINT_SHADOWS };

static constexpr auto MAX_LENSFLARE_TEXTURES{ 16 };

static constexpr auto SSAO_KERNEL_SIZE{ 32 };

}; // namespace ugine
