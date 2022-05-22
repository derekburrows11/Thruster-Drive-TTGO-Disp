
#pragma once




class DriveLink_Serial {
public:
  void Init();
  bool CheckRx();
  void SendTx();



protected:
  bool testDC;
  int txValue;
  int rxdCount;


};

