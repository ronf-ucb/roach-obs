/*
 * Settings
 *
 * created on 2012-5-14 by fgb (derived from or_const.h by apullin)
 */

#ifndef __RADIO_SETTINGS_H
#define __RADIO_SETTINGS_H


#define RONF
// ronf robot radio addresses
#ifdef RONF
#define RADIO_MY_CHAN 0x13
#define RADIO_PAN_ID 0x2060
//Hard code in destination address (basestation) for now, update to be dynamic later
#define RADIO_DEST_ADDR 0x2011
#define RADIO_SRC_ADDR 0x2052
#endif

#ifdef DUNCAN
#define RADIO_MY_CHAN         0x0E
#define RADIO_PAN_ID          0x2100
#define RADIO_DEST_ADDR       0x2101
#define RADIO_SRC_ADDR        0x2102
#endif

#define RADIO_TXPQ_MAX_SIZE   30
#define RADIO_RXPQ_MAX_SIZE   10


#endif //__RADIO_SETTINGS_H
#define TELEM_INCLUDE "vr-telem.h"
