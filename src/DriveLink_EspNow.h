#ifndef _DriveLink_EspNow_h_
#define _DriveLink_EspNow_h_

//#include <inttypes.h>
#include <stdint.h>
#include <esp_now.h>

// ESP_NOW_MAX_DATA_LEN defined as 250 in esp_now.h
#define ESPNOW_MAX_RX_SIZE 100



class DriveLink_EspNowClass {
public:
  void Init();
  bool CheckRx();
  void SendTx();
  uint16_t LastMsgRx() { return msLastMsgRx; }
  void End();

  void ScanWiFi();
  void ConnectKnown();
  void CheckForConnection();
  bool Connected() { return bConnected; }


protected:
  void ConnectSender();
  bool manageConnection();
  int GetRxData(uint8_t *dest, int lenMax);
  void deletePeer();

  static void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);        // called on interrupt
  static void OnDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len);   // called on interrupt


protected:
  uint16_t packetsRxTotal = 0;
  uint16_t packetsRxError = 0;
  uint16_t packetsRxMe = 0;
  uint16_t packetsTx = 0;
  uint16_t msLastMsgSent;
  uint16_t msLastMsgRx;

  static bool bTxOK;
  bool RxOK;
  bool RxTimedOut;
  bool bConnected;
  // Static members because they are used in a static functions
  static bool bRxNewData;
  static bool bRxNewDataReading;
  static bool bRxNewDataMissed;
  static int  iRxNewDataLen;
  static uint8_t buffRx[ESPNOW_MAX_RX_SIZE];

  esp_now_peer_info_t espConn;

  static uint8_t macLastRxd[6];
  static bool bMacLastRxdSet;

};



#endif  // _DriveLink_EspNow_h_
