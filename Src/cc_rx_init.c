/*
 * upsat-comms-software: Communication Subsystem Software for UPSat satellite
 *
 *  Copyright (C) 2016, Libre Space Foundation <http://librespacefoundation.org/>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Rf settings for CC1120
 */
#include <cc_tx_init.h>
#include "cc1120_config.h"

typedef struct
{
	unsigned int   addr;
	unsigned short dat;
}registerSetting_t;




// Whitening = false
// Packet length = 255
// Bit rate = 9.6
// Performance mode = High Performance
// Modulation format = 2-FSK
// RX filter BW = 25.000000
// Deviation = 3.997803
// Packet length mode = Variable
// TX power = 2
// Packet bit length = 0
// Device address = 0
// PA ramping = false
// Carrier frequency = 145.835002
// Symbol rate = 9.6
// Manchester enable = false
// Address config = No address check

static const registerSetting_t RX_preferredSettings[]=
{
  {IOCFG3,            0x00}, /* RXFIFO_THR, on rising edge*/
  {IOCFG2,            0x06}, /* PKT_SYNC_RXTX, on failing edge*/
  {IOCFG1,            0xB0},
  {IOCFG0,            0x06}, /* PKT_SYNC_RXTX, on rising edge*/
  {SYNC1,             0x7A},
  {SYNC0,             0x0E},
  {SYNC_CFG1,         0x0B},
  //{SYNC_CFG0,         0x1C},
  {SYNC_CFG0,         0x0B},
  {DCFILT_CFG,        0x1C},
  {PREAMBLE_CFG1,     0x2A},
  {IQIC,              0xC6},
  {CHAN_BW,           0x08},
  {MDMCFG0,           0x05},
  {SYMBOL_RATE2,      0x73},
  {AGC_REF,           0x20},
  {AGC_CS_THR,        0x19},
  {AGC_CFG1,          0xA9},
  {AGC_CFG0,          0xCF},
  {FIFO_CFG,          CC1120_RXFIFO_THR},
  {FS_CFG,            0x1B},
  {PKT_CFG1,          0x00},
  {PKT_CFG0,          0x20},
  {PA_CFG2,           0x5D},
  {PA_CFG0,           0x7D},
  {PKT_LEN,           0xFF},
  {IF_MIX_CFG,        0x00},
  {FREQOFF_CFG,       0x22},
  {FREQ2,             0x6D},
  {FREQ1,             0x60},
  {FREQ0,             0x52},
  {FS_DIG1,           0x00},
  {FS_DIG0,           0x5F},
  {FS_CAL1,           0x40},
  {FS_CAL0,           0x0E},
  {FS_DIVTWO,         0x03},
  {FS_DSM0,           0x33},
  {FS_DVC0,           0x17},
  {FS_PFD,            0x50},
  {FS_PRE,            0x6E},
  {FS_REG_DIV_CML,    0x14},
  {FS_SPARE,          0xAC},
  {FS_VCO0,           0xB4},
  {XOSC5,             0x0E},
  {XOSC1,             0x03},
  {PARTNUMBER,        0x48},
  {PARTVERSION,       0x21},
  {MODEM_STATUS1,     0x10},
};





void rx_registerConfig() {
	unsigned char writeByte;
	unsigned i;
	// Reset radio
	cc_rx_cmd(SRES);

	// Write registers to radio
	for(i = 0; i < (sizeof(RX_preferredSettings)/sizeof(registerSetting_t)); i++) {
		writeByte = RX_preferredSettings[i].dat;
		cc_rx_wr_reg(RX_preferredSettings[i].addr, writeByte);
	}
}

#define VCDAC_START_OFFSET 2
#define FS_VCO2_INDEX 0
#define FS_VCO4_INDEX   1
#define FS_CHP_INDEX   2





void rx_manualCalibration() {

    uint8_t original_fs_cal2;
    uint8_t calResults_for_vcdac_start_high[3];
    uint8_t calResults_for_vcdac_start_mid[3];
    uint8_t marcstate;
    uint8_t writeByte;

    // 1) Set VCO cap-array to 0 (FS_VCO2 = 0x00)
    writeByte = 0x00;
    cc_rx_wr_reg(FS_VCO2, writeByte);

    // 2) Start with high VCDAC (original VCDAC_START + 2):
    cc_rx_rd_reg(FS_CAL2, &original_fs_cal2);
    writeByte = original_fs_cal2 + VCDAC_START_OFFSET;
    cc_rx_wr_reg(FS_CAL2, writeByte);

    // 3) Calibrate and wait for calibration to be done
    //   (radio back in IDLE state)
    cc_rx_cmd(SCAL);

    do {
        cc_rx_rd_reg(MARCSTATE, &marcstate);
    } while (marcstate != 0x41);

    // 4) Read FS_VCO2, FS_VCO4 and FS_CHP register obtained with
    //    high VCDAC_START value
    cc_rx_rd_reg(FS_VCO2, &calResults_for_vcdac_start_high[FS_VCO2_INDEX]);
    cc_rx_rd_reg(FS_VCO4,&calResults_for_vcdac_start_high[FS_VCO4_INDEX]);
    cc_rx_rd_reg(FS_CHP, &calResults_for_vcdac_start_high[FS_CHP_INDEX]);

    // 5) Set VCO cap-array to 0 (FS_VCO2 = 0x00)
    writeByte = 0x00;
    cc_rx_wr_reg(FS_VCO2, writeByte);

    // 6) Continue with mid VCDAC (original VCDAC_START):
    writeByte = original_fs_cal2;
    cc_rx_wr_reg(FS_CAL2, writeByte);

    // 7) Calibrate and wait for calibration to be done
    //   (radio back in IDLE state)
    cc_rx_cmd(SCAL);

    do {
        cc_rx_rd_reg(MARCSTATE, &marcstate);
    } while (marcstate != 0x41);

    // 8) Read FS_VCO2, FS_VCO4 and FS_CHP register obtained
    //    with mid VCDAC_START value
    cc_rx_rd_reg(FS_VCO2, &calResults_for_vcdac_start_mid[FS_VCO2_INDEX]);
    cc_rx_rd_reg(FS_VCO4, &calResults_for_vcdac_start_mid[FS_VCO4_INDEX]);
    cc_rx_rd_reg(FS_CHP, &calResults_for_vcdac_start_mid[FS_CHP_INDEX]);

    // 9) Write back highest FS_VCO2 and corresponding FS_VCO
    //    and FS_CHP result
    if (calResults_for_vcdac_start_high[FS_VCO2_INDEX] >
        calResults_for_vcdac_start_mid[FS_VCO2_INDEX]) {
        writeByte = calResults_for_vcdac_start_high[FS_VCO2_INDEX];
        cc_rx_wr_reg(FS_VCO2, writeByte);
        writeByte = calResults_for_vcdac_start_high[FS_VCO4_INDEX];
        cc_rx_wr_reg(FS_VCO4, writeByte);
        writeByte = calResults_for_vcdac_start_high[FS_CHP_INDEX];
        cc_rx_wr_reg(FS_CHP, writeByte);
    } else {
        writeByte = calResults_for_vcdac_start_mid[FS_VCO2_INDEX];
        cc_rx_wr_reg(FS_VCO2, writeByte);
        writeByte = calResults_for_vcdac_start_mid[FS_VCO4_INDEX];
        cc_rx_wr_reg(FS_VCO4, writeByte);
        writeByte = calResults_for_vcdac_start_mid[FS_CHP_INDEX];
        cc_rx_wr_reg(FS_CHP, writeByte);
    }
}

