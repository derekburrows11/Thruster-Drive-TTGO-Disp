
//#include <Arduino.h>
#include "config.h"

#include <Utils.h>    // for periodic trigger and performance
#include <MonitorLogColors.h>
#include <Thruster_DataLink.h>

#include "BLE_client.h"
#include "Display.h"
#include "Buttons.h"
#include "BME280.h"
//#include "BMS_Comms.h"


#if IS_VESC_ESP
  #include "Drive_Vesc.h"
  Drive_Vesc vesc;
#endif

#include "DriveLink_BLE.h"
//#include "DriveLink_EspNow.h"
#include "DriveLink_Serial.h"

//#include "Battery_Pack.h"

// Define global instances of classes defined in other files
DriveLink_BLEClass driveLink;
//DriveLink_EspNowClass driveLink;
DriveLink_Serial driveLinkSerial;
//Battery_Pack battPack;

//Define variables for data link
dataController ctrl;
dataDrive drive;




class Display disp;
class buttons butt;
class BME bme;

uint32_t loops = 0;
uint32_t msTimeStart;
int msTime;       // ms
int msTimePrev;   // ms
int secTime;      // seconds
int msChangeMax;  // ms

PeriodicTrigger trigFast(20);       // 50Hz trigger
PeriodicTrigger trigLCD(40);        // 25Hz screen update & for IMU AHRS calculation
PeriodicTrigger trigLCDText(100);   // 10Hz screen text update
PeriodicTrigger trigSec(1000);      // 1Hz trigger
Performance perf;




void setup() {
  Serial.begin(115200);
  Serial.println("Starting Thruster Drive TTGO-Display...");

  butt.init();

  disp.init();

#if IS_VESC_ESP
  vesc.Init();
#endif

  driveLinkSerial.Init();

  driveLink.Init();
//  driveLink.ScanWiFi();

  bme.init();

//  battPack.Init();

#if USE_BLE
  BLE_client_setup();
//  BLE_client_scan();
//  BLE_client_connect();
#error scan and connect not setup properly yet
#endif

  disp.page_change(PAGE_MAIN);
  log_d("Done setup");
}

void resetPerfTimers() {
  perf.reset();
  trigFast.reset();
  trigLCD.reset();
  trigSec.reset();
}

void loop() {
  loops++;
  msTime = millis();
  perf.loop(msTime);

  if (msTime != msTimePrev) {
    msTimePrev = msTime;
    // do polling
  }

  driveLinkSerial.CheckRx();

  if (trigFast.checkTrigger(msTime)) {
//    if (driveLink.CheckRx()) {
      driveLink.CheckRx();
//      digitalWrite(pin_LedAlive, !digitalRead(pin_LedAlive));
#if IS_VESC_ESP
      vesc.SendData();    // ~12bytes at 57.6kbps about  2ms, at 115.2kbps about 1ms
#endif
//    }
  }
  if (trigLCD.checkTrigger(msTime)) {
    butt.loop();
    disp.update();
  }
  if (trigLCDText.checkTrigger(msTime)) {
#if IS_VESC_ESP
      vesc.GetData();     // ~60bytes at 115.2kbps about 5ms
#endif
      driveLink.SendTx();      // send reply
      driveLinkSerial.SendTx();


#if USE_BLE
    BLE_client_loop();
#error
#endif
  }
  if (trigSec.checkTrigger(msTime)) {
    secTime++;
    bme.read();
    
#if IS_VESC_ESP
    vesc.GetMCConfig();     // ~60bytes at 115.2kbps about 5ms
#endif


    if (secTime % 30 == 2)
      vesc.PrintData();
  }



}

