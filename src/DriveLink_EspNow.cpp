
#include "DriveLink_EspNow.h"

//#include <PString.h>
#include <Helpers_Text.h>

//#include <Thruster_Config.h>  // remove this one
#include <Thruster_DataLink.h>

#include <esp_now.h>
#include <WiFi.h>


#include "NoPrint.h"
#undef DEBUGSERIAL
#define DEBUGSERIAL NoPrint

#define CHANNEL 1     // can be different for Tx & Rx
#define PRINTSCANRESULTS 1
#define DELETEBEFOREPAIR 0
#define TIMEOUTms_RX 1000


//void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);
//void OnDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len);

const char *SSIDDrive = "Thruster Drive";
const char *PWDrive = "Drive_1_Password";
// Thruster Drive      [4C:11:AE:7B:C8:94] 94 as STA, 95 as AP ??
// Thruster Controller [24:6F:28:A2:6D:D8] D8 as STA, D9 as AP ??
//static uint8_t macThrusterDrive[6] = {0x4C, 0x11, 0xAE, 0x7B, 0xC8, 0x94};  // Thruster Drive WROOM-32 board
static uint8_t macThrusterCtrl[6]  = {0x24, 0x6F, 0x28, 0xA2, 0x6D, 0xD8};  // Controller WROOM-32 board.  *** Not current ***



// define static member instances
bool DriveLink_EspNowClass::bTxOK;
bool DriveLink_EspNowClass::bRxNewData;
bool DriveLink_EspNowClass::bRxNewDataReading;    // semaphore to Rx callback
bool DriveLink_EspNowClass::bRxNewDataMissed;
int  DriveLink_EspNowClass::iRxNewDataLen;
uint8_t DriveLink_EspNowClass::buffRx[ESPNOW_MAX_RX_SIZE];
uint8_t DriveLink_EspNowClass::macLastRxd[6];
bool DriveLink_EspNowClass::bMacLastRxdSet;



void DriveLink_EspNowClass::Init() {
  Serial.println("Thruster Drive ESPNow Init");

  // For Access Point Mode
  if (!WiFi.mode(WIFI_AP))     // configure in Access Point mode
    Serial.println("WiFi AP Mode Failed");
  if (WiFi.softAP(SSIDDrive, PWDrive, CHANNEL, 0))    // PW need to be at least 8 chars if used
    Serial.println("AP Config Success.  Broadcasting with AP: " + String(SSIDDrive));
  else
    Serial.println("AP Config failed.");
  Serial.print("Drive AP MAC: ");
  Serial.println(WiFi.softAPmacAddress());

/*
  // For Station Mode
  if (!WiFi.mode(WIFI_STA))     // configure in Station mode
    Serial.println("WiFi STA Mode Failed");
  Serial.print("Drive STA MAC: ");
  Serial.println(WiFi.macAddress());
*/

// Init ESPNow
  WiFi.disconnect();
  if (esp_now_init() == ESP_OK)
    Serial.println("ESPNow Init Success");
  else {
    Serial.println("ESPNow Init Failed");
    // Retry esp_now_init(), add a counter and then restart?
    ESP.restart();    // or Simply Restart
  }

  // clear data link structures & connected controller status
  memset(&ctrl,  0, sizeof(ctrl));
  memset(&drive, 0, sizeof(drive));
  bConnected = 0;
  bRxNewData = 0;
  bMacLastRxdSet = 0;


  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);

//  esp_err_t esp_wifi_set_max_tx_power(int8_t power);    // unit is 0.25dBm, range is [40, 82] corresponding to 10dBm - 20.5dBm h
}

void DriveLink_EspNowClass::End() {
  esp_now_deinit();
}


bool DriveLink_EspNowClass::CheckRx() {
  uint16_t msNow = millis();
  if (!bRxNewData) {
    if (msNow - msLastMsgRx > TIMEOUTms_RX) {
      RxTimedOut = 1;
      RxOK = 0;
      ctrl.SetRxTimedOut();
    }
    return 0;
  }

  // Should be a message for us now
  struct dataControllerToDrive fromCtrl;
  uint8_t len = sizeof(fromCtrl);
  packetsRxTotal++;
  RxOK = 0;     // confirm packet is OK first

  int numRx = GetRxData((uint8_t*)&fromCtrl, len);
  if (numRx != len) {
    packetsRxError++;
    DEBUGSERIAL.println("Receive failed");
    DEBUGSERIAL.printf("Expected: %d got %d \n", len, numRx);
    return 0;
  }
  
  DEBUGSERIAL.fprint("Rx Packets Total: ");
  DEBUGSERIAL.print(packetsRxTotal);
  DEBUGSERIAL.print("  len ");
  DEBUGSERIAL.print(len);
  DEBUGSERIAL.print("  ");
  DEBUGSERIAL.print(fromCtrl.id, HEX);
  DEBUGSERIAL.print("  ");
  DEBUGSERIAL.println(fromCtrl.packetCount, HEX);

  // check what sort of packet and from what drive
  if (!(fromCtrl.id & DL_IDbit_DEST_DRIVE1))   // check ID is for drive 1
    return 0;
  if ((fromCtrl.id & DL_IDmask_SRC) != DL_ID_SRC_CTRL)   // check source is control
    return 0;
  if ((fromCtrl.id & DL_IDmask_TYPE) == DL_ID_TYPE_FAST) {   // check ID packet type
//    memcpy(&fromCtrl, buf, sizeof(fromCtrl));
    ctrl.GetData(fromCtrl);
  }
  else
    return 0;

  packetsRxMe++;
  msLastMsgRx = msNow;
  RxTimedOut = 0;
  RxOK = 1;

  drive.rssi = 0;
  drive.packetsRx++;


  DEBUGSERIAL.print("Rx Packets Me: ");
  DEBUGSERIAL.println(packetsRxMe);
//  RH_RF95::printBuffer("Received: ", buf, len);
  DEBUGSERIAL.print("Rxd RSSI: ");
  DEBUGSERIAL.println(drive.rssi, DEC);
  DEBUGSERIAL.print("Ctrl Bat Voltage: ");
  DEBUGSERIAL.println(ctrl.voltageBattery, 3);
//  Serial.print("Rxd Ctrl Throttle: ");
//  Serial.println(ctrl.throttle, 1);
  return 1;
}


void DriveLink_EspNowClass::SendTx() {
  if (!bConnected) {
    ConnectSender();
//    ConnectKnown();
//    CheckForConnection();
    return;
  }
  
  // Send a reply
  struct dataDriveToController fromDrive;
  fromDrive.id = DL_ID_DEST_CTRL | DL_IDbit_SRC_DRIVE1 | DL_ID_TYPE_FAST;
  drive.packetCount++;
  drive.SetData(fromDrive);

  msLastMsgSent = millis();
//  rf95.send((char*)&fromDrive, sizeof(fromDrive));
//  const uint8_t *peer_addr = espConn.peer_addr;
  Serial.print("Sending # bytes: ");
  Serial.println(sizeof(fromDrive));
  esp_err_t result = esp_now_send(espConn.peer_addr, (uint8_t*)&fromDrive, sizeof(fromDrive));
  if (result == ESP_OK)
    Serial.println("Send Success");
  else {
    Serial.printf("Send Failed - code: %d\n", result);
    Serial.println(esp_err_to_name(result));
  }

  DEBUGSERIAL.print(msLastMsgSent);
  DEBUGSERIAL.println("ms  Reply Sent");
}



////////////////////////////////////////
// Protected Functions
////////////////////////////////////////


void DriveLink_EspNowClass::ConnectSender() {
  uint8_t mac[6];
  if (!bMacLastRxdSet)   // mac address not set yet
    return;
  for (int ii = 0; ii < 6; ++ii )
    espConn.peer_addr[ii] = mac[ii] = macLastRxd[ii];

  Serial.printf("Setting peer as %02X:%02X:%02X:%02X:%02X:%02X\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  espConn.channel = CHANNEL; // pick a channel
  espConn.encrypt = 0; // no encryption
  bConnected = manageConnection();
}

void DriveLink_EspNowClass::ConnectKnown() {
  uint8_t mac[6];
  for (int ii = 0; ii < 6; ++ii ) {
    espConn.peer_addr[ii] = mac[ii] = macThrusterCtrl[ii];
//    mac[ii] = macThrusterCtrl[ii];
  }
  Serial.printf("Setting peer as %02X:%02X:%02X:%02X:%02X:%02X\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  espConn.channel = CHANNEL; // pick a channel
  espConn.encrypt = 0; // no encryption
  bConnected = manageConnection();
}

void DriveLink_EspNowClass::CheckForConnection() {

  bConnected = manageConnection();

}



// Scan for espDrives in AP mode
void DriveLink_EspNowClass::ScanWiFi() {
  Serial.print("Scanning WiFi ....");
  int8_t scanResults = WiFi.scanNetworks();
  // reset on each scan
  bool espDriveFound = 0;
  memset(&espConn, 0, sizeof(espConn));

  Serial.println("");
  if (scanResults == 0) {
    Serial.println("No WiFi devices in AP Mode found");
  } else {
    Serial.print("Found "); Serial.print(scanResults); Serial.println(" devices ");
    for (int i = 0; i < scanResults; ++i) {
      // Print SSID and RSSI for each device found
      String SSID = WiFi.SSID(i);
      int32_t RSSI = WiFi.RSSI(i);
      String BSSIDstr = WiFi.BSSIDstr(i);
      if (PRINTSCANRESULTS) {
        Serial.print(i + 1);
        Serial.print(": ");
        Serial.print(SSID);
        Serial.print(" BSSID[");
        Serial.print(BSSIDstr);
        Serial.print("] (rssi ");
        Serial.print(RSSI);
        Serial.print(")");
        Serial.println();
      }
    }
    
    for (int i = 0; i < scanResults; ++i) {
      // Now check for first 'Slave'
      String SSID = WiFi.SSID(i);
      int32_t RSSI = WiFi.RSSI(i);
      String BSSIDstr = WiFi.BSSIDstr(i);
      
      delay(10);
      // Check if the current device starts with has correct SSID
      if (SSID.indexOf(SSIDDrive) == 0) {
        // SSID of interest
        Serial.println("Found a Thruster Drive");
        Serial.print(i + 1); Serial.print(": "); Serial.print(SSID); Serial.print(" ["); Serial.print(BSSIDstr); Serial.print("]"); Serial.print(" ("); Serial.print(RSSI); Serial.print(")"); Serial.println("");
        // Get BSSID => Mac Address of the Slave
        int mac[6];
        if ( 6 == sscanf(BSSIDstr.c_str(), "%x:%x:%x:%x:%x:%x",  &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5] ) ) {
          for (int ii = 0; ii < 6; ++ii ) {
            espConn.peer_addr[ii] = (uint8_t) mac[ii];
          }
        }

        espConn.channel = CHANNEL; // pick a channel
        espConn.encrypt = 0; // no encryption

        espDriveFound = 1;
        // we are planning to have only one espDrive in this example;
        // Hence, break after we find one, to be a bit efficient
        break;
      }
    }
  }

  if (espDriveFound)
    Serial.println("Drive Found");
  else
    Serial.println("Drive Not Found");
  WiFi.scanDelete();    // clean up ram
}




// Check if the espDrive is already paired with the master.
// If not, pair the espDrive with master
bool DriveLink_EspNowClass::manageConnection() {
  if (espConn.channel == CHANNEL) {
    if (DELETEBEFOREPAIR) {
      deletePeer();
    }

    Serial.print("Slave Status: ");
    // check if the peer exists
    bool exists = esp_now_is_peer_exist(espConn.peer_addr);
    if ( exists) {
      // Slave already paired.
      Serial.println("Already Paired");
      return true;
    } else {
      // Slave not paired, attempt pair
      esp_err_t result = esp_now_add_peer(&espConn);
      if (result == ESP_OK) {
        Serial.println("Pair success");
        return true;
      }
// const char *esp_err_to_name(esp_err_t code);
// const char *esp_err_to_name_r(esp_err_t code, char *buf, size_t buflen);
      const char* resultStr = esp_err_to_name(result);
      Serial.println(resultStr);
      if (result == ESP_ERR_ESPNOW_EXIST)
        return true;
      else
        return false;

     }
  } else {
    // No espDrive found to process - different channel
    Serial.println("No Drive found to process, different channel");
    return false;
  }
}


void DriveLink_EspNowClass::deletePeer() {
  esp_err_t result = esp_now_del_peer(espConn.peer_addr);
  if (result == ESP_OK)
    Serial.println("Delete Slave Success");
  else {
    Serial.printf("Delete Slave Failed - code: %d\n", result);
    Serial.println(esp_err_to_name(result));
  }
}

int DriveLink_EspNowClass::GetRxData(uint8_t *dest, int lenMax) {
  if (!bRxNewData)    // no new data rxd
    return 0;
  bRxNewDataReading = 1;    // semaphore to Rx callback
  int len = min(iRxNewDataLen, lenMax);
  memcpy(dest, buffRx, len);
  bRxNewData = 0;
  bRxNewDataReading = 0;
  return len;
}

// callback after data is sent - async
void DriveLink_EspNowClass::OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  bTxOK = (status == ESP_NOW_SEND_SUCCESS);

  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
//  if (bTxOK) Serial.print("OnDataSent OK CB to: ");
//  else Serial.print("OnDataSent Failed to: ");
//  Serial.println(macStr);
}


// callback when data is recv - async
void DriveLink_EspNowClass::OnDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len) {
  if (bRxNewDataReading) {    // don't change 'buffRx' if currently being read!
    bRxNewDataMissed = 1;
    return;
  }
  // Store mac addr of sender - only for creating a new cnnection to reply
  for (int ii = 0; ii < 6; ++ii )
    macLastRxd[ii] = mac_addr[ii];
  bMacLastRxdSet = 1;

  
  int len = min(data_len, (int)sizeof(buffRx));
  memcpy(buffRx, data, len);   // check length less than buffer
  iRxNewDataLen = len;
  bRxNewData = 1;

  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
//  Serial.printf("Recv %d bytes from: %s\n", data_len, macStr);
//  Serial.printf("Data: %d %d %d %d %d   \n", data[0], data[1], data[2], data[3], data[4]);
}




const char* esp_Err_Str(esp_err_t code) {
// const char *esp_err_to_name(esp_err_t code);
// const char *esp_err_to_name_r(esp_err_t code, char *buf, size_t buflen);
  const char* str = NULL;
  switch (code) {
  case ESP_OK:
    str = "OK"; break;
  case ESP_ERR_ESPNOW_NOT_INIT:
    str = "ESPNOW Not Init"; break;
  case ESP_ERR_ESPNOW_ARG:
    str = "Invalid Argument"; break;
  case ESP_ERR_ESPNOW_FULL:
    str = "Peer list full"; break;
  case ESP_ERR_ESPNOW_NO_MEM:
    str = "Out of memory"; break;
  case ESP_ERR_ESPNOW_INTERNAL:
    str = "Internal Error"; break;
  case ESP_ERR_ESPNOW_EXIST:
    str = "Peer Exists"; break;
  case ESP_ERR_ESPNOW_NOT_FOUND:
    str = "Peer not found";  break;
  default:
    str = "Not sure!!";
  }
  return str;
}
