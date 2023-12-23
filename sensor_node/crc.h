#ifndef CRC_H
#define CRC_H

#include "Arduino.h"

int calc_SAE_J1850(uint8_t data[], int crc_len);

#endif
