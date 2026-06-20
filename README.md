# LCC_16_Input_Node

  - OpenLCB 16-Input Node – ESP32 Version
  - Version: 1.0
  - Date: June 2026
  - Author: John Holmes (based on David P Harris maintaind example.)

## Overview

This node provides 16 configurable digital inputs using an ESP32. Each input can generate standard OpenLCB On-Events and Off-Events, supporting various input types including toggle mode with delays.

Features

  - 16 independent input channels
  - 6 input types per channel:
  - None
  - Input (active high)
  - Input Inverted
  - Input with Pull-up
  - Input with Pull-up Inverted
  - Toggle
  - Toggle with Pull-up

  - Configurable On-delay and Off-delay (0–25.5 seconds)

Full OpenLCB Producer support with correct userState() responses (VALID=4, INVALID=5, UNKNOWN=7)

CDI configuration via JMRI or compatible tools

Factory reset support (Not supported at this stage)

## Hardware

  - MCU: ESP32 DevKit (or compatible)
  - Inputs: GPIO pins defined in Config.h
  - CAN Bus: transceiver (pins 15 RX, 2 TX)
  - Optional: Direct USB connection via #define USEGCSERIAL (not setup without altering the config.h file.)

Pin Configuration (Current)

  - #define NUM_IO   16
  - #define IOPINS   14,27,26,32,15,4,16,23,22,21,17,5,18,19,33,25

## How It Works

Event Mapping

  - Each input channel has 2 events:
  - Even index → On-Event
  - Odd index  → Off-Event

  - Total: 32 events

userState() Logic

uint8_t userState(uint16_t index) {
    int ch = index / 2;
    if (channel not configured) return UNKNOWN (7);
    
    uint8_t desired = (index % 2) ? 1 : 0;   // 0=ON event, 1=OFF event
    return (desired == current_state) ? VALID (4) : INVALID (5);
}

Input Processing

  - Scans inputs round-robin
  - Supports immediate events or delayed events
  - Toggle mode flips internal logic state on falling edge

Configuration (CDI)

  - Each of the 16 inputs has:

  - Description (24 chars)
  - Input Type (dropdown)
  - On-delay (0–255 × 100ms)
  - Off-delay (0–255 × 100ms)
  - On-Event ID
  - Off-Event ID

Setup & Flashing

  - Set RESET_TO_FACTORY_DEFAULTS 1 first time
  - Adjust NODE_ADDRESS to be unique
  - Flash using Arduino IDE 
  - After first boot, set reset flag back to 0

Files

  - Config.h – Main configuration
  - Main .ino file
  - Acan_ESP32_Can
  - Boards
  - README
 
  
  ## Required libraries: 
  
  OpenLCB_Single_Thread this needs to be installed before this sketch will compile. It is available via the Arduino Library manager.






