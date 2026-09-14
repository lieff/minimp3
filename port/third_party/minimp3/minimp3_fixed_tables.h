#pragma once
/* minimp3.h includes its tables by the path they have inside FastLED's tree.
   Forwarding here is what lets the decoder header stay byte-identical between
   the two repositories, which is the property tools/check_vendor_sync.py
   enforces and the whole reason this directory exists. */
#include "../../../minimp3_fixed_tables.h"
