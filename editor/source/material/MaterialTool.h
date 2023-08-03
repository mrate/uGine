#pragma once

#include <ugine/engine/gfx/Material.h>

#include <ugine/String.h>

#define MATERIAL_RESOURCE_ICON ICON_FA_PALETTE

namespace ugine::ed {

SerializedMaterial InstantiateMaterial(ResourceHandle<Material> material, StringView name);

}