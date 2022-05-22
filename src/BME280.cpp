
#include <Thruster_DataLink.h>
#include "config.h"
#include "BME280.h"

#define SEALEVELPRESSURE_HPA (1013.25)

extern int secTime;
BME280_Plus bme280;


void BME280_Plus::readBurstPTH(int32_t& adc_P, int32_t& adc_T, int16_t& adc_H) {
    uint8_t reg = BME280_REGISTER_PRESSUREDATA;    // Pressure 3 bytes, Temperature 3 bytes, Humidity 2 bytes
    if (i2c_dev) {            // is on I2C
        _wire->beginTransmission(_i2caddr);
        _wire->write(reg);
        _wire->endTransmission();
        _wire->requestFrom(_i2caddr, (uint8_t)8);        // 8 bytes total
        adc_P = _wire->read();
        adc_P <<= 8;
        adc_P |= _wire->read();
        adc_P <<= 8;
        adc_P |= _wire->read();

        adc_T = _wire->read();
        adc_T <<= 8;
        adc_T |= _wire->read();
        adc_T <<= 8;
        adc_T |= _wire->read();

        adc_H = (_wire->read() << 8) | _wire->read();

    } else {            // is on SPI
        _spi->beginTransaction(SPISettings(500000, MSBFIRST, SPI_MODE0));
        digitalWrite(_cs, LOW);
        spixfer(reg | 0x80); // read, bit 7 high
        adc_P = spixfer(0);
        adc_P <<= 8;
        adc_P |= spixfer(0);
        adc_P <<= 8;
        adc_P |= spixfer(0);

        adc_T = spixfer(0);
        adc_T <<= 8;
        adc_T |= spixfer(0);
        adc_T <<= 8;
        adc_T |= spixfer(0);

        adc_H = (spixfer(0) << 8) | spixfer(0);

        digitalWrite(_cs, HIGH);
        _spi->endTransaction(); // release the SPI bus
    }
}




void BME::init() {

    // use non default pins
    bool resPins = Wire.setPins(SDA_BME280, SCL_BME280);
//    Wire.setClock(20000);              // 100kHz is default
    Serial.printf("setPins result: %d \n", resPins);
//    pinMode(GND_BME280, OUTPUT);                // set power 0V pin low
//    pinMode(POS_BME280, OUTPUT);                // set power 3.3V pin high
//    digitalWrite(GND_BME280, 0);
//    digitalWrite(POS_BME280, 1);
//    delay(500);

    Serial.printf("BME gnd pin is %d, pos is %d \n", digitalRead(GND_BME280), digitalRead(POS_BME280));


    bme280.setTwoWire(&Wire);
    bool resultOK = bme280.begin(BME280_ADDRESS_ALTERNATE);      // BME I2C address either 0x77 or 0x76


    if (resultOK) {
        Serial.print("Found BME280, sensorID is: 0x");
        Serial.println(bme280.sensorID(), 16);         // BME280 is 0x60, BMP280 is 0x56-58
    } else {
        Serial.println("Could not find a valid BME280 sensor, check wiring, address, sensor ID!");
        Serial.print("SensorID was: 0x");
        Serial.println(bme280.sensorID(), 16);
        Serial.print("   ID of 0xFF probably means a bad address, a BMP 180 or BMP 085\n");
        Serial.print("   ID of 0x56-0x58 represents a BMP 280,\n");
        Serial.print("   ID of 0x60 represents a BME 280.\n");
        Serial.print("   ID of 0x61 represents a BME 680.\n");
    }

}


void BME::read() {

    if (secTime != secTimePrev) {
        secTimePrev = secTime;


    // or better, burst read all 3+3+2 bytes
/*    int32_t adc_P;
    int32_t adc_T;
    int16_t adc_H;  
    bme280.readBurstPTH(adc_P, adc_T, adc_H);
*/

    drive.BMETemperature = bme280.readTemperature();   // deg C
//    if (drive.BMEtemperature == NAN)       // failled to read - try resetting I2C
    drive.BMEPressure = bme280.readPressure();         // Pa
    drive.BMEHumidity = bme280.readHumidity();         // % 
    drive.BMEDewPoint = dewPointFast1(drive.BMETemperature, drive.BMEHumidity);   // deg C

//        uint8_t outVal = secTime & 0x01;
//        digitalWrite(GND_BME280, outVal);
//        digitalWrite(POS_BME280, outVal);
    //    delay(10);
    //    Serial.printf("BME gnd pin is %d, pos is %d, wrote %d \n", digitalRead(GND_BME280), digitalRead(POS_BME280), outVal);

        Serial.printf("BME pres, temp, hum, dewP is %.0f, %.2f, %.1f, %.1f\n", drive.BMEPressure, drive.BMETemperature, drive.BMEHumidity, drive.BMEDewPoint);

    }

}

float BME::dewPointFast2(float celsius, float humidity) {
    float a = 17.271;
    float b = 237.7;
    float temp = log(humidity * 0.01) + ((a * celsius) / (b + celsius));
    float dewPoint = (b * temp) / (a - temp);
    return dewPoint;
}
float BME::dewPointFast3(float celsius, float humidity) {
	//iMV is the intermidate Value 
	float iMV = (log(humidity / 100) + ((17.27 * celsius)/(237.3 + celsius))) / 17.27;
	float dewPoint = (237.3 *iMV) / (1-iMV);
	return dewPoint;
}
float BME::dewPointFast1(float celsius, float humidity) {        // from wikipedia - dew point
    float dewDiff;
    if (humidity >= 85)
        dewDiff = 0.133 * (100.0 - humidity);   // approx 1 deg C diff in temp and dewpoint is 5% RH from 50% to 100%
    else if (humidity >= 50)
        dewDiff = 0.2 * (95.0 - humidity);
    else
        dewDiff = 0.2 * (95.0 - humidity);      // check
    return (celsius - dewDiff);
}
