#pragma once


uint16_t byteRev(uint16_t val) {uint8_t* pByte = (uint8_t*)&val; return (uint16_t)word(pByte[0], pByte[1]); }

#define get_uint16(pVal)            (*(uint16_t*)(pVal))
#define getRev_uint16(pVal)  byteRev(*(uint16_t*)(pVal))


struct BMSMsgHead {
  uint8_t headmark;     // 0xdd
  uint8_t msgType;      // 3= BMS Info, 4= Cell Voltages, 5= Version string
  uint8_t error;        // 0=OK, 0x80=error
  uint8_t dataLength;   // bytes of data following, up to not including checksum

  bool GetData(uint8_t data[]) {
    headmark    = data[0];
    msgType     = data[1];
    error       = data[2];
    dataLength  = data[3];
    if (headmark != 0xdd)
      return 0;
    return 1;
  }
};

struct BMSMsgTail {
  uint16_t checksum;    // byte reversed.  Should sum to 0x0000 with data after first two bytes
  uint8_t tailmark;     // 0x77

  bool GetData(uint8_t data[]) {
    checksum    = getRev_uint16(data - 3);
    tailmark    = data[-1];
    if (tailmark != 0x77)
      return 0;
    return 1;
  }
};

union BMSProtStat {
  uint16_t Status;
  struct  {
    bool cellOV;  // over voltage
    bool cellUV;  // under voltage
    bool batteryOV;
    bool batteryUV;
    bool chargeOT;  // over temperature
    bool chargeUT;  // under temperature
    bool dischargeOT;
    bool dischargeUT;
    
    bool chargeOC;    // over current
    bool dischargeOC;
    bool dischargeSC; // short circuit
    bool hardwareError;
    bool MOSSoftwareLock;
  };
};

struct BMSMOSControl {
  bool shutdownChargeMOS;
  bool shutdownDischargeMOS;
};
struct BMSMOSStatus {
  bool ChargeMOS;
  bool DischargeMOS;
};

union MOSFETControlByte {
  uint8_t byteVal;    //  bit 0=charge, bit 1=discharge, 1 is shutdown
  struct BMSMOSControl MOSControl;
};



struct BMSData_Info {
  uint16_t calcCS;
  uint8_t dataLength;   // bytes of data following, up to not including checksum
  // byte 4
  float voltageTotal;       // raw unit 10mV
  float current;            // raw unit 10mA
  float capacityResidual;   // raw unit 10mAh
  float capacityNominal;    // raw unit 10mAh
  float cycleLife;      // 
  uint16_t productDate;    // 
  // byte 16
  uint16_t balanceStatus;      // cells 1-16, 1 is on
  uint16_t balanceStatusHigh;  // cells 17-32, 1 is on
//  uint16_t protectionStatus;   // note
  union BMSProtStat prot;
/*  union {
    uint16_t protectionStatus;
    struct {
      bool cellOV;  // over voltage
      bool cellUV;  // under voltage
    } prot;
  };
*/

  uint8_t  version;
  float  RSOC;            // raw unit 1% of the residual capacity
  // byte 24
  //  BMSMOSStatus; // MOSStatus;
  union {
    uint8_t  MOSFETStatus;    //  bit 0=charge, bit 1=discharge, 0 is off
    struct {
      bool ChargeMOS;
      bool DischargeMOS;
    };
  };

  uint8_t  numCells;        // number of cells
  uint8_t  numNTC;          // number of NTCs
  float tempNTC1;       // raw unit 0.1deg, absolute (+273.1)
  float tempNTC2;       // raw unit 0.1deg, absolute (+273.1)
  // byte 31

  void GetData(uint8_t data[]) {
    dataLength        = data[3];
    voltageTotal      = getRev_uint16(data +  4) * 0.01;   // 10mV
    current           = getRev_uint16(data +  6) * 0.01;   // 10mA
    capacityResidual  = getRev_uint16(data +  8) * 0.01;   // 10mAh
    capacityNominal   = getRev_uint16(data + 10) * 0.01;   // 10mAh
    cycleLife         = getRev_uint16(data + 12);          // 
    productDate       = getRev_uint16(data + 14);   // 
    balanceStatus     = getRev_uint16(data + 16);   // cells 1-16, 1 is on
    balanceStatusHigh = getRev_uint16(data + 18);   // cells 17-32, 1 is on
    prot.Status       = getRev_uint16(data + 20);   // note
    version           = data[22];
    RSOC              = data[23];            // % of the residual capacity
    MOSFETStatus      = data[24];
    numCells          = data[25];
    numNTC            = data[26];
    tempNTC1          = getRev_uint16(data + 27) * 0.1 - 273.1;       // *0.1deg K (+273.1)
    tempNTC2          = getRev_uint16(data + 29) * 0.1 - 273.1;       // *0.1deg K (+273.1)

    calcCS = 0;
    for (int i = 0; i < dataLength; i++)
      calcCS += data[i];
  }

  void serialPrint() {
    Serial.printf("Voltage %.2f V, Current %.2f A\n", voltageTotal, current);
    Serial.printf("capacityResidual %.2f Ah, capacityNominal %.2f Ah \n", capacityResidual, capacityNominal);
  
  
    Serial.printf("RSOC %.1f %%\n", RSOC);
    Serial.printf("MOSFETs - Charge: %d  Discharge: %d\n", (MOSFETStatus&1) != 0, (MOSFETStatus&2) != 0);
    Serial.printf("Num Cells %d, Num NTCs %d\n", numCells, numNTC);
    Serial.printf("tempNTC1 %.1f C, tempNTC2 %.1f C\n", tempNTC1, tempNTC2);

    if (prot.cellOV) Serial.print(", Cell OV");
  //  if (MOSStatus.ChargeMOS) Serial.print(", Charge On");
    if (MOSFETStatus) Serial.print(", Charge On");
    if (ChargeMOS) Serial.print(", Charge On");

  }
};

struct BMSData_VoltageCells {
  uint16_t calcCS;
  uint8_t dataLength;   // bytes of data following, up to not including checksum
  // byte 4
  float VoltageCell[16]; // cell mV, byte reversed.  Length depending on #cells

  void GetData(uint8_t data[]) {
    dataLength  = data[3];
    for (int i = 0; i < 16; i++)
      VoltageCell[i]      = getRev_uint16(data+4 + 2*i) * 0.001;   // 1mV

    calcCS = 0;
    for (int i = 0; i < dataLength; i++)
      calcCS += data[i];
  }
  
  void serialPrint() {
    for (int i = 0; i < 12; i++)
      Serial.printf("Cell[%d]  %.3f V\n", i+1, VoltageCell[i]);
  }
};

struct BMSData_Version {
  uint16_t calcCS;
  uint8_t dataLength;   // bytes of data following, up to not including checksum
  // byte 4
  char zVersion[32]; // up to 31 ascii characters

  void GetData(uint8_t data[]) {
    dataLength  = data[3];
    memcpy(zVersion, data+4, dataLength);
    zVersion[dataLength] = 0;

    calcCS = 0;
    for (int i = 0; i < dataLength; i++)
      calcCS += data[i];
  }

  void serialPrint() {
      Serial.println(zVersion);
  }

};
