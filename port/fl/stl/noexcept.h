#pragma once
/* FL_NO_EXCEPT is a no-op in FastLED on every platform -- the decoder is built
   with -fno-exceptions, so marking functions noexcept would buy nothing and on
   some toolchains costs a terminate handler. Keeping it as an empty macro here
   preserves that, and keeps the decoder source byte-identical. */
#define FL_NO_EXCEPT
