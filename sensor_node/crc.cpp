#include "crc.h"

int calc_SAE_J1850(uint8_t data[], int crc_len) {
    int idx, crc, temp1, temp2, idy = 0;

    crc = 0;
    for (idx = 0; idx < crc_len + 1; idx++) {
    	if (idx == 0) temp1 = 0;
    	else temp1 = data[crc_len - idx];

        crc = (crc ^ temp1);
        for (idy = 8; idy > 0; idy--) {
            temp2 = crc;
            crc <<= 1;
            if ((temp2 & 128) != 0) {
                crc ^= 0x1D;
            }
        }
    }
    return crc;
}