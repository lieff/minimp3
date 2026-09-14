#pragma once
/* The inline and optimisation attributes the decoder's performance depends on.
   These are not cosmetic: FASTLED_FORCE_INLINE has to beat -fno-inline (the
   operation audit builds with it), and FL_OPTIMIZE_FUNCTION is what puts the
   DCT-32 and IMDCT kernels at -O3 inside an -Os build. Weakening any of them
   silently costs double-digit percentages on the target. */
#if defined(__GNUC__) || defined(__clang__)
#define FASTLED_FORCE_INLINE inline __attribute__((always_inline))
#define FL_ALWAYS_INLINE     static inline __attribute__((always_inline))
#define FL_NO_INLINE         __attribute__((noinline))
/* `optimize` is a GCC attribute; clang parses it and ignores it with a
   warning, so ask clang only for `hot`. The kernels this marks are why the
   decoder can be built -Os and still run its DCT-32 and IMDCT at -O3 -- on
   clang builds they are simply at the file's optimisation level. FastLED's
   shipping targets use GCC. */
#if defined(__clang__)
#define FL_OPTIMIZE_FUNCTION __attribute__((hot))
#else
#define FL_OPTIMIZE_FUNCTION __attribute__((optimize("O3"), hot))
#endif
#else
#define FASTLED_FORCE_INLINE inline
#define FL_ALWAYS_INLINE     static inline
#define FL_NO_INLINE
#define FL_OPTIMIZE_FUNCTION
#endif
