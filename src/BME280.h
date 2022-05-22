#pragma once

//#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>    // There is also Adafruit_BMP280.h library, but only BME280 has humidity


// extension of Adafruit_BME280 library class from 2021.  Library changed late 2021 to use Adafuit BusIO
class BME280_Plus : public Adafruit_BME280 {
public:
  void readBurstPTH(int32_t& adc_P, int32_t& adc_T, int16_t& adc_H);

  // to maintiain functionallity of earlier library - simpler!
  void setTwoWire(TwoWire* theWire = &Wire) { _wire = theWire; }
  uint8_t spixfer(uint8_t x) { return _spi->transfer(x); }
protected:  
  TwoWire*  _wire = NULL;
  SPIClass* _spi = NULL;
  int8_t _cs;   //!< for the SPI interface
};



class BME {
  int secTimePrev;

public:
  void init();
  void read();
  float dewPointFast1(float celsius, float humidity);
  float dewPointFast2(float celsius, float humidity);
  float dewPointFast3(float celsius, float humidity);
};
