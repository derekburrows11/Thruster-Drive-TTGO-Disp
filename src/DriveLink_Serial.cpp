
#include "config.h"
#include <Arduino.h>

#include <Thruster_DataLink.h>

#include "DriveLink_Serial.h"
/*
class HardwareSerial_Ext  {
public:
  friend class HardwareSerial;
  uart_t* getUart() { return _uart; }
};
*/

void DriveLink_Serial::Init() {
  testDC = 0;
  digitalWrite(TXE_COMMS_RL, 0);
  pinMode(TXE_COMMS_RL, OUTPUT);

  if (!testDC) {
//    SERIALRL.begin(115200, SERIAL_8N1, RXD_COMMS_RL, TXD_COMMS_RL);   // for TTGO T-Display ESP32, 115200 or 57600
    SERIAL_RL.begin(115200, SERIAL_8N1, TXD_COMMS_RL, TXD_COMMS_RL);   // use same pin

  } else {
    digitalWrite(TXD_COMMS_RL, 0);
    pinMode(TXD_COMMS_RL, OUTPUT);
    pinMode(RXD_COMMS_RL, INPUT);
  }

}

void DriveLink_Serial::SendTx() {   // send up to 1ms max at one time, about 10 bytes
//  char msg[] = "abcd";
  txValue++;
  if (!testDC) {
//    digitalWrite(TXE_COMMS_RL, 0);        // disable output driver to allow Radio Link to send

    digitalWrite(TXD_COMMS_RL, 1);      // keeps output high when detaching
    pinMatrixOutDetach(TXD_COMMS_RL, 0, 0);
    pinMode(TXD_COMMS_RL, INPUT);       // this no effect if done before detaching

    delayMicroseconds(100);
    pinMode(TXD_COMMS_RL, INPUT_PULLUP);
    digitalWrite(TXE_COMMS_RL, 0);

    delayMicroseconds(100);
    pinMode(TXE_COMMS_RL, INPUT);

    delayMicroseconds(200);
    pinMode(TXD_COMMS_RL, INPUT_PULLUP);

    delayMicroseconds(100);
    pinMode(TXD_COMMS_RL, INPUT);

    delayMicroseconds(100);
    pinMode(TXE_COMMS_RL, OUTPUT);

    delayMicroseconds(100);
    digitalWrite(TXE_COMMS_RL, 1);
    pinMode(TXE_COMMS_RL, INPUT);

    delayMicroseconds(100);
//    pinMode(TXD_COMMS_RL, OUTPUT);    // don't need to reassign to output
    pinMatrixOutAttach(TXD_COMMS_RL, U1TXD_OUT_IDX, 0, 0);    // SERIALRL, UART_TXD_IDX(1)

//    digitalWrite(TXE_COMMS_RL, 1);        // enable output driver
    SERIAL_RL.write(0x00);
    SERIAL_RL.write((uint8_t)txValue);
    SERIAL_RL.write(0x00);
//    SERIALRL.write(msg, 2);

    delayMicroseconds(500);
    pinMode(TXE_COMMS_RL, OUTPUT);
    digitalWrite(TXE_COMMS_RL, 0);


  } else {
    digitalWrite(TXD_COMMS_RL, (txValue >> 1) & 0x01);        // set output state
    digitalWrite(TXE_COMMS_RL, (txValue >> 2) & 0x01);        // enable output driver if high
  }


}


bool DriveLink_Serial::CheckRx() {
  if (!SERIAL_RL.available())
    return 0;
  char rx = SERIAL_RL.read();
  rxdCount++;
  rx = rx;
//  Serial.printf("%x ", rx);
  return 1;
}
