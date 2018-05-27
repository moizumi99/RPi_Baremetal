#include "helper.h"
#include "mylib.h"

uint32_t cut_bits(uint8_t *csd, int32_t start, int32_t end)
{
  uint32_t ret = 0;
  int32_t shift = 0;
  int32_t bits = 0;
  for(int32_t i = 0; i < 16 && 0 <= end; i++) {
    uint8_t mask;
    if (start < 8) {
      if (8 <= end) {
        mask = 0xff;
        bits = 8 - start;
      } else {
        mask = ((0xFF << (7 - end)) & 0xFF) >> (7 - end);
        bits = end - start + 1;
      }
      ret |= ((csd[15 - i] >> start) & mask) << shift;
      shift += bits;
      start = 0;
      end -= 8;
    } else {
      start -= 8;
      end -= 8;
    }
    /* printf("Ret: %4x, mask: %x, start: %d, end: %d, shift: %d\n", ret, mask, start, end, shift); */
  }
  return ret;
}

void cut_bit_test()
{
    uint8_t csd[16];
    csd[3] = csd[4] = 0xff;
    int32_t result = cut_bits(csd, 22, 37);
    if ( result == 0xFFFC ) {
        printf("Cut bits test pass\n");
    } else {
        printf("Cut bits test fail. Expected %d, actual %d\n", 0xFFFC, result);
    }
    result = cut_bits(csd, 22, 35);
    if ( result == 0x3FFC ) {
        printf("Cut bits test pass\n");
    } else {
        printf("Cut bits test fail. Expected %d, actual %d\n", 0x3FFC, result);
    }
}



