#ifndef _Drive_Vesc_h_
#define _Drive_Vesc_h_
//#pragma once


//#define DEBUG

//Library for VESC UART
#include <VescUartExtra.h>

extern mc_configuration driveMCConfig;


class Drive_Vesc {
public:
  void Init();

  void SendData();
  void PrintData();

  // Functions that wait for response
  void GetData();
  void GetMCConfig();

  // Functions to request and poll for response
  void RequestData();
  void RequestMCConfig();
  int CheckRxd();



  const char* GetFaultString();

  VescUartExtra vescComm;

  int batteryNumCells;
  int motorNumPoles;

protected:
  int iNumBytesRet;
  int readConfigSet;
  
};

#endif  // _Drive_Vesc_h_
