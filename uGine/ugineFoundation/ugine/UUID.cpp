#include "UUID.h"

#ifdef WIN32

#include <Windows.h>

namespace ugine {

UUID UUID::Generate() {
    GUID guid{};
    CoCreateGuid(&guid);

    static_assert(sizeof(GUID) == sizeof(ugine::UUID));

    UUID result{};
    memcpy(&result.v, &guid, sizeof(GUID));
    return result;
}

} // namespace ugine

#endif
