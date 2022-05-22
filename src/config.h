// Config for Thruster Drive on TTGO-Display (ttgo-t1)

#define IS_RADIOLINK_ESP  0    // 1 for Radio Link ESP, 0 for Vesc comms ESP
#define IS_VESC_ESP       1    // 0 for Radio Link ESP, 1 for Vesc comms ESP

#define USE_BLE 0
#define DISP_USE_SPRITE 0


#include "Buttons.h"
extern class Performance perf;
extern class Display disp;
extern class buttons butt;

// extern mc_configuration driveMCConfig;


// examples from M5 controller...
//extern M5Display& lcd;
//extern M5Touch& touch;
//extern M5Buttons& buttons;

void resetPerfTimers();

// #define LED_BUILTIN 22    // is 22 for TTGO-T1 - not for TTGO-T_Display

#define PIN_BUTTON1        0
#define PIN_BUTTON2        35


//  All these are defined in <User_Setups/Setup25_TTGO_T_Display.h> anyway
#define TFT_MOSI            19
#define TFT_SCLK            18
#define TFT_CS              5
#define TFT_DC              16
#define TFT_RST             23
#define TFT_BL              4   // 4 = Display backlight control pin


// USART defaults
#define RXD0 3
#define TXD0 1
#define RXD1 22
#define TXD1 23
#define RXD2 16
#define TXD2 17

// Serial 0 for USB port
// Serial 1 for radio link ESP32 - Drive Link
// Serial 2 for VESC

#define DEBUGSERIAL  Serial
#define SERIAL_RL    Serial1
#define SERIAL_VESC  Serial2

// define USART ports and pins used.  For VESC and comms to surface radio
/* 7 pin cable to VESC
  Black  - 5V
  White  - Gnd
  Orange - 27 (Rxd for ESP32)
  Green  - 26 (Txd for ESP32)
*/ 
#define RXD_VESC GPIO_NUM_27
#define TXD_VESC GPIO_NUM_26

// USART for comms to radio link ESP32
#define TXD_COMMS_RL GPIO_NUM_32
#define RXD_COMMS_RL GPIO_NUM_33
#define TXE_COMMS_RL GPIO_NUM_25


// Two Wire I2C for BME280 uses sda=38, scl=37, 36 = low output for power supply

//#define GND_BME280 GPIO_NUM_36  // input only
//#define SCL_BME280 GPIO_NUM_37  // input only
//#define SDA_BME280 GPIO_NUM_38  // input only

#define POS_BME280 GPIO_NUM_2   // pin 2 effects bootloader if held high a little during boot, and stops download conecting!!!
#define GND_BME280 GPIO_NUM_17
#define SCL_BME280 GPIO_NUM_22
#define SDA_BME280 GPIO_NUM_21

// maybe if on top left with 3.3V wired across
//#define SCL_BME280 GPIO_NUM_21
//#define SDA_BME280 GPIO_NUM_22

