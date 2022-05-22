
#include "config.h"
//#include <Arduino.h>

#include <Helpers_Text.h>
#include <NoPrint.h>
//#include <Thruster_Config.h>  // remove this one
#include <Thruster_DataLink.h>

#include "Drive_Vesc.h"
#include "VescUartExtra.h"

//#define DEBUGSERIAL NoPrint

mc_configuration driveMCConfig;   // defined in VescUart/datatypes.h


void Drive_Vesc::Init() {
  SERIAL_VESC.begin(115200, SERIAL_8N1, RXD_VESC, TXD_VESC);   // for TTGO T-Display ESP32
//  SERIALIO.begin(117000);   // 32U4 is running slower.  Output baud rate 111 kBaud instead of 115.2 kBaud
//  SERIALIO.begin(57600);   // 115.2 is too fast for 32U4 at 8MHz

  vescComm.setSerialPort(&SERIAL_VESC);
//  vescComm.setDebugPort(&DEBUGSERIAL);    // don't set serial port, too much debug info normally!

  batteryNumCells = 12;
  motorNumPoles = 7;
}

void Drive_Vesc::RequestData() {
  vescComm.RequestVescValues();        // 90us/byte, 11.5 bytes/ms @115.2kbps about 5ms for 60 bytes


}

void Drive_Vesc::RequestMCConfig() {
  vescComm.RequestMCConfig();


}

int Drive_Vesc::CheckRxd() {
  vescComm.CheckRxd();

}

void Drive_Vesc::GetData() {
//Getting Values from Vesc over UART
//  bool bGotVals = VescUartGetValue(VescMeasuredValues);
/* VESC 4.20 (reports Hw:410) with Firmware 5.2 (8/2021) returns 73 bytes.  6.34ms @115.2kbps
*/

  iNumBytesRet = vescComm.getVescValuesSize();      //160us/byte, 6 bytes/ms, 60 bytes/10ms @57.6kbps, @115.2kbps about 5ms
//  VescUart::dataPackage& VescMeasuredValues = vescComm.data;

// Store values to transmit to controller
  drive.tempMosfet      = vescComm.data.tempMosfet;
  drive.tempMotor       = vescComm.data.tempMotor;
  drive.currentMotor    = vescComm.data.avgMotorCurrent;
  drive.currentBattery  = vescComm.data.avgInputCurrent;
  drive.rpm             = vescComm.data.rpm / motorNumPoles;      // erpm = #magnet poles(7) * rpm
//  drive.rpm             = vescComm.data.rpmMotor;      // erpm = #magnet poles(7) * rpm
  drive.dutyCycle       = vescComm.data.dutyCycleNow;
  drive.voltageBattery  = vescComm.data.inpVoltage;
  drive.fault           = vescComm.data.error;
  drive.chargeBattery   = (drive.voltageBattery / batteryNumCells - 3.3) * (100 / 0.9);   // 3.3 -> 4.2 V/cell

  drive.ampHours        = vescComm.data.ampHours;
  drive.ampHoursCharged = vescComm.data.ampHoursCharged;

}

void Drive_Vesc::GetMCConfig() {

  switch (++readConfigSet) {
  case 0:
    break;
  case 1:
//    log_d("reading vescComm.getMCConfig");
    iNumBytesRet = vescComm.getMCConfig(driveMCConfig);   // 90us/byte, 11.5 bytes/ms @115.2kbps about 5ms for 60 bytes
//    log_d("vescComm.getMCConfig len: %d ", iNumBytesRet);
    log_d("Vesc fault: %d", drive.fault);
    break;
  case 2:
    //log_d("reading vescComm.getFWversion");
    iNumBytesRet = vescComm.getFWversion();
    log_d("vescComm.getFWversion ret: %d , FW%d.%d", iNumBytesRet, vescComm.fw_version.major, vescComm.fw_version.minor);
    break;
  default:
    readConfigSet = 0;
  }

}


void Drive_Vesc::PrintData() {
  DEBUGSERIAL.println();
  DEBUGSERIAL.fprintln("--------- Get Vesc data -------- ");

  Serial.fprint("Battery V: ");
  Serial.println(drive.voltageBattery, 2);

#ifdef DEBUG
  DEBUGSERIAL.fprint("Got payload from UART: ");    // reading 65 bytes of payload, msg ID + 64 bytes
  DEBUGSERIAL.println(iNumBytesRet);
  if (iNumBytesRet >= 56) {
    SerialPrint(VescMeasuredValues);
  } else {
    DEBUGSERIAL.fprint("Got <56 from UART: ");
    DEBUGSERIAL.println(iNumBytesRet);
  }
  DEBUGSERIAL.fprint("Battery %: ");
  DEBUGSERIAL.println(drive.chargeBattery, 1);
  DEBUGSERIAL.fprint("Fault: ");
  DEBUGSERIAL.println(GetFaultString());
  
  DEBUGSERIAL.fprintln("--------- End Vesc data -------- ");
#endif

}

void Drive_Vesc::SendData() {
  float pcThrottle = ctrl.throttle;
  float factThrottle = pcThrottle * 0.01;
  if (pcThrottle >= 0.1)
  {
//    VescUartSetCurrent(factThrottle * 30.0);
    vescComm.setDuty(factThrottle);
//    VescUartSetCurrentBrake(factThrottle * 30.0);

//    DEBUGSERIAL.fprint("Throttle Sent % ");
//    DEBUGSERIAL.println(pcThrottle);
  }
  else if (pcThrottle < -1.0)
  {
    vescComm.setBrakeCurrent(pcThrottle * -0.1);   // 10A brake at 100%
//    DEBUGSERIAL.fprint("Brake % ");
//    DEBUGSERIAL.println(pcThrottle);
  }
  else
  {
//    DEBUGSERIAL.fprintln("Current & Brake 0");
    vescComm.setCurrent(0.0);
//    VescUartSetCurrentBrake(0.0);
  }
}


const char* Drive_Vesc::GetFaultString() {
  const char* pStr;
  // Version 2014 faults - old
  // fault enum in VescUartControl\datatypes.h - enum mc_fault_code
  switch (drive.fault) {
    case FAULT_CODE_NONE:     // None = 0
      pStr = "None"; break;
    case FAULT_CODE_OVER_VOLTAGE:
      pStr = "Over Voltage"; break;
    case FAULT_CODE_UNDER_VOLTAGE:
      pStr = "Under Voltage"; break;
    case FAULT_CODE_DRV:
      pStr = "MOSFET Drv8302"; break;
    case FAULT_CODE_ABS_OVER_CURRENT:
      pStr = "Over Current"; break;
    case FAULT_CODE_OVER_TEMP_FET:
      pStr = "MOSFET Temp"; break;
    case FAULT_CODE_OVER_TEMP_MOTOR:
      pStr = "Motor Temp"; break;
    default:
      pStr = "Not Defined";
  }
  return pStr;
}
