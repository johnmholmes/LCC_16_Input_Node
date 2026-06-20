/* 16input_v1.0.1

This is my test version for demonstration of a CAN Bus node only by John Holmes
  - Pins 15 RX and 2 TX for the transceiver module
  - Pins D4,D16,D17,D5,D18,D19,D21,D22,D13,D12,D14,D27,D26,D25,D33,D32  are used for input
*/

/*
==============================================================
 Based of the AVR 2Servos NIO using ESPcan

 Coprright 2024 David P Harris
 derived from work by Alex Shepherd and David Harris
 Updated 2024.11.14 DPH
 Modifyed by John Holmes June 2026 for 16 inputs
==============================================================
 - 16 input/output channels:
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
#include "Boards.h"   // Contains Board defintions, see "Boards.h"

// User defs
#define OLCB_NO_BLUE_GOLD

#include "mdebugging.h"           // debugging
#include "OpenLCBHeader.h"        // System house-keeping.

// CDI (Configuration Description Information) in xml, must match MemStruct
// See: http://openlcb.com/wp-content/uploads/2016/02/S-9.7.4.1-ConfigurationDescriptionInformation-2016-02-06.pdf
extern "C" {
    #define N(x) xN(x)     // allow the insertion of the value (x) ..
    #define xN(x) #x       // .. into the CDI string.
const char configDefInfo[] PROGMEM =
// ===== Enter User definitions below =====
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
            <name>On-delay 1-255 = 100ms-25.5s, 0=steady-state</name>
            <hints><slider tickSpacing='50' immediate='yes' showValue='yes'> </slider></hints>
        </int>
        <int size='1'>
            <name>Off-delay 1-255 = 100ms-25.5s, 0=No repeat</name>
            <hints><slider tickSpacing='50' immediate='yes' showValue='yes'> </slider></hints>
        </int>
        <eventid><name>On-Event</name></eventid>
        <eventid><name>Off-Event</name></eventid>
    </group>
    )" CDIfooter;
// ===== Enter User definitions above =====
} // end extern

// ===== MemStruct =====
//   Memory structure of EEPROM, must match CDI above
    typedef struct {
          EVENT_SPACE_HEADER eventSpaceHeader; // MUST BE AT THE TOP OF STRUCT - DO NOT REMOVE!!!
          char nodeName[20];  // optional node-name, used by ACDI
          char nodeDesc[24];  // optional node-description, used by ACDI
      // ===== Enter User definitions below =====
          struct {
            char desc[24];
            uint8_t type;
            uint8_t duration;    // 100ms-25.5s, 0=solid
            uint8_t period;      // 100ms-25.5s, 0=no repeat
            EventID onEid;
            EventID offEid;
          } io[NUM_IO];
      // ===== Enter User definitions above =====
      
      // items below will be included in the EEPROM, but are not part of the CDI
    } MemStruct;                 // type definition

extern "C" {
    // ===== eventid Table =====
    //  Array of the offsets to every eventID in MemStruct/EEPROM/mem, and P/C flags
    const EIDTab eidtab[NUM_EVENT] PROGMEM = {
        IOEID(NUM_IO)
    };
    
    // SNIP Short node description for use by the Simple Node Information Protocol
    // See: http://openlcb.com/wp-content/uploads/2016/02/S-9.7.4.3-SimpleNodeInformation-2016-02-06.pdf
    extern const char SNII_const_data[] PROGMEM = "\001" MANU "\000" MODEL "\000" HWVERSION "\000" OlcbCommonVersion ; // last zero in double-quote
} // end extern "C"

// PIP Protocol Identification Protocol uses a bit-field to indicate which protocols this node supports
// See 3.3.6 and 3.3.7 in http://openlcb.com/wp-content/uploads/2016/02/S-9.7.3-MessageNetwork-2016-02-06.pdf
uint8_t protocolIdentValue[6] = {   //0xD7,0x58,0x00,0,0,0};
        pSimple | pDatagram | pMemConfig | pPCEvents | !pIdent    | pTeach     | !pStream   | !pReservation, // 1st byte
        pACDI   | pSNIP     | pCDI       | !pRemote  | !pDisplay  | !pTraction | !pFunction | !pDCC        , // 2nd byte
        0, 0, 0, 0                                                                                           // remaining 4 bytes
    };

uint8_t iopin[] = { IOPINS };
bool iostate[NUM_IO] = {0};  // state of the iopin
bool logstate[NUM_IO] = {0}; // logic state for toggle
unsigned long next[NUM_IO] = {0};

// This is called to initialize the EEPROM to Factory Reset
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

// determine the state of each eventid
enum evStates { VALID=4, INVALID=5, UNKNOWN=7 };

uint8_t userState(uint16_t index) {
    // iostate: indicates whether the channel is ON (1) or OFF(0).
    int ch = index / 2;  // two event indices per channel

    if (NODECONFIG.read(EEADDR(io[ch].type)) == 0) {
        return UNKNOWN;
    }

    uint8_t eidstate = (index % 2) ? 1 : 0;  // even eventids = ON, odd eventids = OFF

    if (eidstate == iostate[ch]) {
        return VALID;                        // implied state matches actual state
    }
    
    return INVALID;
}

// ===== Process Consumer-eventIDs =====
void pceCallback(uint16_t index) {
}

void printMem();

// ==== Process Inputs ====
void produceFromInputs() {
    // called from loop(), this looks at changes in input pins and
    // and decides which events to fire
    // with pce.produce(i);

    static uint8_t c = 0;
    static unsigned long last = 0;
    if((millis()-last)<(50/NUM_IO)) return;
    last = millis();
    uint8_t type = NODECONFIG.read(EEADDR(io[c].type));
    uint8_t d;
    if(type==5 || type==6) {
      bool s = digitalRead(iopin[c]);
      if(s != iostate[c]) {
        iostate[c] = s;
        if(!s) {
          logstate[c] ^= 1;
          if(logstate[c]) d = NODECONFIG.read(EEADDR(io[c].duration));
          else            d = NODECONFIG.read(EEADDR(io[c].period));
          //dP("\ninput "); PV(c); PV(type); PV(s); PV(logstate[c]); PV(d);
          if(d==0) OpenLcb.produce(logstate[c] ); // if no delay send the event
          else next[c] = millis() + (uint16_t)d*100;          // else register the delay
          //PV(millis()); PV(next[c]);
        }
      }
    }
    if(type>0 && type<5) {
      bool s = digitalRead(iopin[c]);
      if(s != iostate[c]) {
        iostate[c] = s;
        if(!iostate[c]) d = NODECONFIG.read(EEADDR(io[c].duration)); 
        else d = NODECONFIG.read(EEADDR(io[c].period));
        //dP("\ninput "); PV(type); PV(s); PV(d);
        if(d==0) OpenLcb.produce(!s^(type&1) ); // if no delay send event immediately
        else {
          next[c] = millis() + (uint16_t)d*100;                   // else register the delay
          //PV(millis()); PV(next[c]);
        }
      }
    }
    if(++c>=NUM_IO) c = 0;
}

// Process pending producer events
// Called from loop to service any pending event waiting on a delay
void processProducer() {

}

void userSoftReset() {}
void userHardReset() {}

NodeID nodeid(NODE_ADDRESS);  // this node's nodeid, do not move
#include "OpenLCBMid.h"    // Essential, do not move or delete

// Callback from a Configuration write
// Use this to detect changes in the ndde's configuration
// This may be useful to take immediate action on a change.
void userConfigWritten(uint32_t address, uint16_t length, uint16_t func)
{
  EEPROMcommit;

  setupIOPins();

}

// Setup the io pins
// called by setup() and after a configuration change
void setupIOPins() {
  dP("\nPins: ");
  for(uint8_t i=0; i<NUM_IO; i++) {
    uint8_t type = NODECONFIG.read( EEADDR(io[i].type));
    switch (type) {
      case 1: case 2: case 5:
        dP(" IN:");
        pinMode(iopin[i], INPUT); 
        iostate[i] = type&1;
        if(type==5) iostate[i] = 0;
        break;
      case 3: case 4: case 6:
        dP(" INP:");
        pinMode(iopin[i], INPUT_PULLUP); 
        iostate[i] = type&1;
        break;

    }
    dP(iopin[i]); dP(":"); dP(type); dP(", ");
  }
}

// ==== Setup does initial configuration ======================
void setup()
{
  //#ifdef DEBUG
    Serial.begin(115200); while(!Serial);
    delay(2000);
    dP("\n HiHo");
  //#endif
  EEPROMbegin;
  NodeID nodeid(NODE_ADDRESS);       // this node's nodeid
  dP("\nRESET_TO_FACTORY_DEFAULTS="); dP(RESET_TO_FACTORY_DEFAULTS);
  Olcb_init(nodeid, RESET_TO_FACTORY_DEFAULTS);
 
  dP("\n MemStruct size= "); dP((uint16_t)sizeof(MemStruct));

  setupIOPins();
  dP("\n setup NUM_EVENT="); dP(NUM_EVENT);
}

// ==== Loop ==========================
void loop() {
  bool activity = Olcb_process();
  produceFromInputs();  // scans inputs and generates events on change
  processProducer();    // processes delayed producer events from inputs
}

