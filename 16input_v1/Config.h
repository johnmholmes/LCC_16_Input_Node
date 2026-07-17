#ifndef CONFIG_H
#define CONFIG_H

// To Force Reset EEPROM to Factory Defaults set this value to 1, else 0.
// Need to do this at least once.
#define RESET_TO_FACTORY_DEFAULTS 1

// To set a new nodeid edit the next line
#define NODE_ADDRESS  0x05,0x01,0x01,0x01,0x8E,0x03 // must be unique from an address space owned by you or DIY

// Choose a board, uncomment one line, see boards.h
#define ESP32_BOARD
//#define ATOM_BOARD

// Debugging -- uncomment to activate debugging statements:
//#define DEBUG Serial

// Allow direct to JMRI via USB, without CAN controller, comment out for CAN
//#define USEGCSERIAL
//#define NOCAN

#define NUM_IO   16
#define IOPINS    4,16,17,5,18,19,21,22,13,12,14,27,26,25,33,32
#define NUM_EVENT (NUM_IO*2)

// Board definitions
#define MANU "OpenLCB"           // The manufacturer of node
#define MODEL BOARD "16 input"  // The model of the board
#define HWVERSION "ESP Basic"          // Hardware version
#define SWVERSION "1.0.2"          // Software version

#ifdef USEGCSERIAL
  #include "GCSerial.h"
  #undef DEBUG           // Cannot use DEBUG when using GCSerial
#endif

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

// Definitions for EIDTab
// These allow automatic regostering of eventids
// Expands depending on NUM_IO

#define REG_IO(i) PCEID(io[i].onEid), PCEID(io[i].offEid)

#define _IOEID_1 REG_IO(0)
#define _IOEID_2 _IOEID_1, REG_IO(1)
#define _IOEID_3 _IOEID_2, REG_IO(2)
#define _IOEID_4 _IOEID_3, REG_IO(3)
#define _IOEID_5 _IOEID_4, REG_IO(4)
#define _IOEID_6 _IOEID_5, REG_IO(5)
#define _IOEID_7 _IOEID_6, REG_IO(6)
#define _IOEID_8 _IOEID_7, REG_IO(7)
#define _IOEID_9 _IOEID_8, REG_IO(8)
#define _IOEID_10 _IOEID_9, REG_IO(9)
#define _IOEID_11 _IOEID_10, REG_IO(10)
#define _IOEID_12 _IOEID_11, REG_IO(11)
#define _IOEID_13 _IOEID_12, REG_IO(12)
#define _IOEID_14 _IOEID_13, REG_IO(13)
#define _IOEID_15 _IOEID_14, REG_IO(14)
#define _IOEID_16 _IOEID_15, REG_IO(15)
#define _IOEID(n) _IOEID_##n
#define IOEID(n) _IOEID(n)

#endif // CONFIG_H

