#include "Profile.h"

#include <ugine/Memory.h>

#ifdef UGINE_PROFILE_TRACY
namespace ugine::profile {
tracy::VkCtx* TracyVkContext{};
}
#endif
