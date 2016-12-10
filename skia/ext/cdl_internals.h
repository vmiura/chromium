#ifndef SKIA_EXT_CDL_INTERNALS_H_
#define SKIA_EXT_CDL_INTERNALS_H_

#include <stdint.h>

union CdlPaintBits{
  struct {
      // all of these bitfields should add up to 32
      unsigned        flags : 16;
      unsigned        text_align : 2;
      unsigned        cap_type : 2;
      unsigned        join_type : 2;
      unsigned        style : 2;
      unsigned        text_encoding : 2;  // 3 values
      unsigned        hinting : 2;
      unsigned        filter_quality : 2;
      //unsigned      fFreeBits : 2;
  } bitfields;
  uint32_t bitfields_uint;
};

#endif // SKIA_EXT_CDL_INTERNALS_H_
