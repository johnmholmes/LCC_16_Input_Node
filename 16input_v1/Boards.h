// Board definitions

#if defined(ESP32_BOARD)
  #define BOARD "ESP32"
  #define NUM_IO    16
  #define IOPINS     4,16,17,5,18,19,21,22,13,12,14,27,26,25,33,32
  #define CAN_TX_PIN (gpio_num_t) 2
  #define CAN_RX_PIN (gpio_num_t) 15
  #ifndef USEGCSERIAL
    #include "ACAN_ESP32Can.h"
  #endif // USEGCSERIAL
  #define EEPROMSIZE 4096
  #define EEPROMbegin { EEPROM.begin(EEPROMSIZE); dP("\nEEPROM begin "); dP(EEPROMSIZE)
  #define EEPROMcommit { EEPROM.commit(); dP("EEPROM COMMIT"); }

#elif defined(ATOM_BOARD)
  #define BOARD "Atom"
  #define NUM_IO    4
  #define IOPINS    22, 19, 23, 33
  #define CAN_TX_PIN (gpio_num_t)26
  #define CAN_RX_PIN (gpio_num_t)32
  #ifndef USEGCSERIAL
    #include "ACAN_ESP32Can.h"
  #endif // USEGCSERIAL
  #define EEPROMSIZE 4096
  #define EEPROMbegin EEPROM.begin(EEPROMSIZE); // this sets the size of the emulated eeprom. 
  #define EEPROMcommit EEPROM.commit();


#else
  #error "No board selected"

#endif
