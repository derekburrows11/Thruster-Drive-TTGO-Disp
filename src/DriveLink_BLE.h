#pragma once

//#include <inttypes.h>
#include <stdint.h>

#define ESPNOW_MAX_RX_SIZE 100


class DriveLink_BLEClass {
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


  static uint8_t macLastRxd[6];
  static bool bMacLastRxdSet;

};

