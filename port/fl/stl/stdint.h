#pragma once
/* Fixed-width integer types in namespace fl, matching FastLED's own shim.
   FastLED defines these itself because some embedded toolchains disagree about
   <stdint.h>; here the platform always has one, so this just re-exports. */
#include <stdint.h>
#include <stddef.h>

namespace fl {
typedef ::int8_t   i8;
typedef ::uint8_t  u8;
typedef ::int16_t  i16;
typedef ::uint16_t u16;
typedef ::int32_t  i32;
typedef ::uint32_t u32;
typedef ::int64_t  i64;
typedef ::uint64_t u64;
typedef ::size_t   size;
typedef ::size_t   size_t;
} // namespace fl
