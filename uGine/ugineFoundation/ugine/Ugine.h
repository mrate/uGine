#pragma once

#include <cassert>

#include <stdint.h>

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using f32 = float;
using f64 = double;

static constexpr u32 UGINE_MIN_THREADS{ 4 };
static constexpr u32 UGINE_MAX_THREADS{ 32 };

#define UGINE_ASSERT(x) assert(x)
#define UGINE_FATAL(msg) abort()

#ifdef _DEBUG
#define UGINE_BREAK ::ugine::Break()
#else
#define UGINE_BREAK
#endif

#ifdef _MSC_VER
#define UGINE_FORCE_INLINE __forceinline
#else
static_assert(false, "Platform not implemented");
#endif

namespace ugine {
void Break();
}