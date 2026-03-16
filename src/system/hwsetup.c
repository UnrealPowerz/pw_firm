#include "all_headers.h"

// ROM: 0xb0f2  97.9%
void HardwareSetup(void)
{
  CKSTPR2 |= 0x10;
  SSCRL |= 0x40;
  SSMR = 0x86;
  SSCRH = 0x8C;
  PUCR9 = 0x08;
  PCR9 = 0x01;
  PCR1 = 0x07;
  PDR1 = 0x05;
  PDR9 |= 0x01;
  PMRB = 0x01;
}
