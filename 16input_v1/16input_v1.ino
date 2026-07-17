/* 16input_v1.0.2

This is my test version for demonstration of a CAN Bus node only by John Holmes
  - Pins 15 RX and 2 TX for the transceiver module
  - Pins D4,D16,D17,D5,D18,D19,D21,D22,D13,D12,D14,D27,D26,D25,D33,D32  are used for input
*/

/*
==============================================================
 Based of the AVR 2Servos NIO using ESPcan

 Copyright 2024 David P Harris
 derived from work by Alex Shepherd and David Harris
 Updated 2024.11.14 DPH
 Modified by John Holmes June 2026 for 16 inputs
==============================================================
 - 16 input channels:
     - type: 0=None, 
             1=Input, 2=Input inverted, 
             3=Input with pull-up, 
             4=Input with pull-up inverted, 
             5=Input toggle, 
             6=Toggle with pull-up
     - Inputs:
       - Events are produced
       - On-delay: delay before on-event is sent
       - Off-delay: the period before the off-event is sent
==============================================================
*/

#include "Config.h"   // Contains configuration, see "Config.h"
#include "Boards.h"   // Contains Board definitions, see "Boards.h"

// User defs
#define OLCB_NO_BLUE_GOLD

#include "mdebugging.h"           // debugging
#include "OpenLCBHeader.h"        // System house-keeping.

// CDI (Configuration Description Information) in xml, must match MemStruct
extern "C" {
    #define N(x) xN(x)
    #define xN(x) #x
const char configDefInfo[] PROGMEM =
  CDIheader R"(
    <name>Application Configuration</name>
    <group replication=')" N(NUM_IO) R"('>
        <name>Inputs as they appear on the ESP32 Devkit 1 sensor shield</name>
        <repname>Input D4 </repname>
        <repname>Input D16 </repname>
        <repname>Input D17 </repname>
        <repname>Input D5 </repname>
        <repname>Input D18 </repname>
        <repname>Input D19 </repname>
        <repname>Input D21 </repname>
        <repname>Input D22 </repname>
        <repname>Input D13 </repname>
        <repname>Input D12 </repname>
        <repname>Input D14 </repname>
        <repname>Input D27 </repname>
        <repname>Input D26 </repname>
        <repname>Input D25 </repname>
        <repname>Input D33 </repname>
        <repname>Input D32 </repname>
        <string size='24'><name>Description</name></string>
        <int size='1'><name> Choose an input type from the drop down list</name>
            <name>Channel Type type</name>
            <map>
                <relation><property>0</property><value>None</value></relation> 
                <relation><property>1</property><value>Input</value></relation> 
                <relation><property>2</property><value>Input Inverted</value></relation> 
                <relation><property>3</property><value>Input with pull-up</value></relation>
                <relation><property>4</property><value>Input with pull-up Inverted</value></relation>
                <relation><property>5</property><value>Toggle</value></relation>
                <relation><property>6</property><value>Toggle with pull-up</value></relation>
            </map>
        </int>
        <int size='1'>
            <name>On-delay 0=steady-state, 1-255 = From 100ms to 25.5 seconds </name>
            <hints><slider tickSpacing='85' immediate='yes' showValue='yes'> </slider></hints>
        </int>
        <int size='1'>
            <name>Off-delay  0=No repeat, 1-255 = from 100ms to 25.5 seconds,</name>
            <hints><slider tickSpacing='50' immediate='yes' showValue='yes'> </slider></hints>
        </int>
        <eventid><name>HIGH State 3.3 volts - Event</name></eventid>
        <eventid><name>LOW State 0 Volts - Event</name></eventid>
    </group>
    )" CDIfooter;
} // end extern

// ===== MemStruct =====
typedef struct {
    EVENT_SPACE_HEADER eventSpaceHeader;
    char nodeName[20];
    char nodeDesc[24];
    struct {
        char desc[24];
        uint8_t type;
        uint8_t duration;    // 100ms-25.5s, 0=solid
        uint8_t period;      // 100ms-25.5s, 0=no repeat
        EventID onEid;
        EventID offEid;
    } io[NUM_IO];
} MemStruct;

extern "C" {
    const EIDTab eidtab[NUM_EVENT] PROGMEM = {
        IOEID(NUM_IO)
    };
    
    extern const char SNII_const_data[] PROGMEM = "\001" MANU "\000" MODEL "\000" HWVERSION "\000" SWVERSION " " OlcbCommonVersion;
}

uint8_t protocolIdentValue[6] = {
    pSimple | pDatagram | pMemConfig | pPCEvents | !pIdent | pTeach | !pStream | !pReservation,
    pACDI   | pSNIP     | pCDI       | !pRemote  | !pDisplay | !pTraction | !pFunction | !pDCC,
    0, 0, 0, 0
};

uint8_t iopin[] = { IOPINS };
bool iostate[NUM_IO] = {0};
bool logstate[NUM_IO] = {0};
unsigned long next[NUM_IO] = {0};
uint16_t pendingEvent[NUM_IO] = {0};   // NEW: stores which event to send after delay

// Factory Reset
void userInitAll()
{ 
    NODECONFIG.put(EEADDR(nodeName), ESTRING("Esp32"));
    NODECONFIG.put(EEADDR(nodeDesc), ESTRING("16 input"));

    for(uint8_t i = 0; i < NUM_IO; i++) {
        NODECONFIG.put(EEADDR(io[i].desc), ESTRING(""));
        NODECONFIG.update(EEADDR(io[i].type), 0);
        NODECONFIG.update(EEADDR(io[i].duration), 0);
        NODECONFIG.update(EEADDR(io[i].period), 0);
    }  
    EEPROMcommit;
}

enum evStates { VALID=4, INVALID=5, UNKNOWN=7 };

uint8_t userState(uint16_t index) {
    int ch = index / 2;
    if (NODECONFIG.read(EEADDR(io[ch].type)) == 0) {
        return UNKNOWN;
    }
    uint8_t eidstate = (index % 2) ? 1 : 0;  // even = ON, odd = OFF
    if (eidstate == iostate[ch]) {
        return VALID;
    }
    return INVALID;
}

void pceCallback(uint16_t index) {}

void printMem() {}

// ====================== INPUT PROCESSING ======================
void produceFromInputs() {
    static uint8_t c = 0;
    static unsigned long last = 0;
    if((millis()-last) < (50/NUM_IO)) return;
    last = millis();

    uint8_t type = NODECONFIG.read(EEADDR(io[c].type));
    if(type == 0) {
        if(++c >= NUM_IO) c = 0;
        return;
    }

    bool s = digitalRead(iopin[c]);
    uint16_t baseIndex = (uint16_t)c * 2;

    if(type==5 || type==6) {                    // Toggle
        if(s != iostate[c]) {
            iostate[c] = s;
            if(!s) {                            // trigger on falling edge
                logstate[c] ^= 1;
                uint8_t d = logstate[c] ? 
                    NODECONFIG.read(EEADDR(io[c].duration)) :
                    NODECONFIG.read(EEADDR(io[c].period));

                uint16_t evt = baseIndex + (logstate[c] ? 0 : 1);

                if(d==0) {
                    OpenLcb.produce(evt);
                } else {
                    next[c] = millis() + (uint16_t)d * 100;
                    pendingEvent[c] = evt;
                }
            }
        }
    }
    else if(type > 0 && type < 5) {             // Normal inputs
        if(s != iostate[c]) {
            iostate[c] = s;
            bool logicHigh = !s ^ (type & 1);   // apply inversion

            uint8_t d = logicHigh ? 
                NODECONFIG.read(EEADDR(io[c].duration)) :
                NODECONFIG.read(EEADDR(io[c].period));

            uint16_t evt = baseIndex + (logicHigh ? 0 : 1);

            if(d==0) {
                OpenLcb.produce(evt);
            } else {
                next[c] = millis() + (uint16_t)d * 100;
                pendingEvent[c] = evt;
            }
        }
    }

    if(++c >= NUM_IO) c = 0;
}

// Process delayed events
void processProducer() {
    unsigned long now = millis();
    for(uint8_t i = 0; i < NUM_IO; i++) {
        if(next[i] && now >= next[i]) {
            next[i] = 0;
            if(pendingEvent[i] != 0) {
                OpenLcb.produce(pendingEvent[i]);
                pendingEvent[i] = 0;
            }
        }
    }
}

void userSoftReset() {}
void userHardReset() {}

NodeID nodeid(NODE_ADDRESS);

#include "OpenLCBMid.h"

// Configuration changed
void userConfigWritten(uint32_t address, uint16_t length, uint16_t func)
{
    EEPROMcommit;
    setupIOPins();
}

// Setup IO pins
void setupIOPins() {
    dP("\nPins: ");
    for(uint8_t i=0; i<NUM_IO; i++) {
        uint8_t type = NODECONFIG.read(EEADDR(io[i].type));
        switch (type) {
            case 1: case 2: case 5:
                pinMode(iopin[i], INPUT); 
                iostate[i] = (type==5) ? 0 : (type&1);
                break;
            case 3: case 4: case 6:
                pinMode(iopin[i], INPUT_PULLUP); 
                iostate[i] = (type&1);
                break;
            default:
                pinMode(iopin[i], INPUT); // safe default
        }
        dP(iopin[i]); dP(":"); dP(type); dP(", ");
    }
    dP("\n");
}

// ====================== SETUP ======================
void setup()
{
    Serial.begin(115200); 
    while(!Serial);
    delay(2000);
    dP("\n HiHo - 16 Input Node v1.0.2");

    EEPROMbegin;
    NodeID nodeid(NODE_ADDRESS);
    dP("\nRESET_TO_FACTORY_DEFAULTS="); dP(RESET_TO_FACTORY_DEFAULTS);
    Olcb_init(nodeid, RESET_TO_FACTORY_DEFAULTS);
 
    dP("\n MemStruct size= "); dP((uint16_t)sizeof(MemStruct));

    setupIOPins();
}

// ====================== LOOP ======================
void loop() {
    Olcb_process();
    produceFromInputs();   // scan inputs
    processProducer();     // handle delayed events
}
