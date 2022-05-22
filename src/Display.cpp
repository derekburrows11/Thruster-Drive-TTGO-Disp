
//#include <Arduino.h>
#include "config.h"

// Loading of TFT_eSPI user setup done in platformio,ini with
// build_flags = -include "C:\Users\user\.platformio\lib\TFT_eSPI\User_Setups\Setup25_TTGO_T_Display.h"
//#include <User_Setups/Setup25_TTGO_T_Display.h>    // Setup file for ESP32 and TTGO T-Display ST7789V SPI bus TFT - from <User_Setup_Select.h>
//#define USER_SETUP_LOADED
#include <TFT_eSPI.h>

#include "Display.h"

#include <Thruster_DataLink.h>
//#include <VescUart.h>   // VescUart library is from https://github.com/RollingGecko/VescUartControl

#include "Drive_Vesc.h"
//extern mc_configuration driveMCConfig;


#define lcd tft
TFT_eSPI tft = TFT_eSPI(TFT_WIDTH, TFT_HEIGHT);  // are constructor defaults anyway 135x240



#define DISPLAY_MARGIN_LEFT 40
#define DISPLAY_MARGIN_VALUES 140
#define DISPLAY_MARGIN_UNITS  210
#define DISPLAY_LINESPACE  2


struct buttonInfo srcnButton[] = {{0, 0, 10, 40, 4, NULL, TFT_RED, TFT_MAROON}, {0, 95, 10, 40, 4, NULL, TFT_BLUE, TFT_NAVY}};


#include <Utils.h>    // for periodic trigger
extern PeriodicTrigger trigFast;       // 50Hz trigger - to check for zeroing
extern PeriodicTrigger trigLCD;        // 25Hz screen update - to check for zeroing
extern PeriodicTrigger trigSec;      // 1Hz trigger - period gets zeroed when creating sprite



//! Long time delay, it is recommended to use shallow sleep, which can effectively reduce the current consumption
void espDelay(int ms)
{
    esp_sleep_enable_timer_wakeup(ms * 1000);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
    esp_light_sleep_start();
}


void Display::init() {
    lcd.init();
    lcd.setRotation(3);
    lcd.fillScreen(TFT_BLACK);
    lcd.setTextSize(4);     // 2 initially
    lcd.setTextColor(TFT_GREEN);
    lcd.setCursor(0, 0);
    lcd.setTextDatum(MC_DATUM);     // MC_DATUM or TL_DATUM

    if (TFT_BL > 0) { // TFT_BL has been set in the TFT_eSPI library in the User Setup file TTGO_T_Display.h
        pinMode(TFT_BL, OUTPUT); // Set backlight pin to output mode
        digitalWrite(TFT_BL, TFT_BACKLIGHT_ON); // Turn backlight on. TFT_BACKLIGHT_ON has been set in the TFT_eSPI library in the User Setup file TTGO_T_Display.h
    }

    page_setupStartupScreen();
}


void Display::pushSprite() {
#if DISP_USE_SPRITE
  lcd.pushSprite(0, 0);
#else
      // nothing needed
#endif
}
void Display::fill(uint16_t clr) {
#if DISP_USE_SPRITE
    lcd.fillSprite(clr);
#else
  lcd.fillScreen(clr);
#endif
}


void Display::update() {
  page_update();
  pushSprite();
}




void Display::page_changeNext() {
  if (++pageCurr >= PAGE_LAST)
    pageCurr = 0;
  pageCurrSub = 0;
  page_change(pageCurr);
}
void Display::page_changePrev() {
  if (--pageCurr < 0)
    pageCurr = PAGE_LAST - 1;
  pageCurrSub = 0;
  page_change(pageCurr);
}
void Display::page_changeSubNext() {
  if (++pageCurrSub > pageSubs[pageCurr])
    pageCurrSub = pageSubs[pageCurr];
  if (pageSubs[pageCurr] != 0)
    page_change(pageCurr);
}
void Display::page_changeSubPrev() {
  if (--pageCurrSub < 0)
    pageCurrSub = 0;
  if (pageSubs[pageCurr] != 0)
    page_change(pageCurr);
}
void Display::page_change(int page) {
  pageCurr = page;
  GfxElem_Num = 0;    // remove GfxElem's
  msgNum = 0;

  page_setup();
}



void Display::page_setup() {
  fill(TFT_BLACK);
  lcd.setTextFont(1);
  lcd.setTextSize(2);
  lcd.setTextWrap(0);
  statusBar_Show = 1;

  switch (pageCurr + pageCurrSub * PAGE_SUB1) {
  case PAGE_MAIN:
    page_setupMain(); break;
  case PAGE_DRIVEFB:
    //page_setupDriveFB();
     break;
  case PAGE_TEMPS:
    page_setupTemps(); break;
  case PAGE_MCCONFIG:
    page_setupMCConfig(); break;
  case PAGE_CTRLPOWER:
    //page_setupCtrlPower();
     break;
   case PAGE_CTRLPOWER + PAGE_SUB1:
    //page_setupCtrlPowerS1();
     break;
 case PAGE_TIMING:
    page_setupTiming(); break;
  case PAGE_MEMORY:
    page_setupMemory(); break;
  default:
    ;
  }
  page_setup_statusBar();
}


void Display::page_update() {
  lcd.setTextFont(1);   // 4 ?
  lcd.setTextSize(2);
  lcd.setTextWrap(0);
  lineNum = 0;
  updateTextNow = (++updateTextCycle >= updateTextMax);
  if (updateTextNow) {
    updateTextCycle = 0;
    if (statusBar_Show)
      page_updateText_statusBar();
  }

  if (updateTextNow)
    switch (pageCurr + pageCurrSub*PAGE_SUB1) {
    case PAGE_MAIN:
      page_updateMain(); break;
    case PAGE_DRIVEFB:
      //page_updateDriveFB();
      break;
    case PAGE_TEMPS:
      page_updateTemps(); break;
    case PAGE_MCCONFIG:
      page_updateMCConfig(); break;
    case PAGE_CTRLPOWER:
      //page_updateCtrlPower();
      break;
    case PAGE_MEMORY:
      page_updateMemory(); break;
    }
  
// if any Gfx
//  for (int i = 0; i < GfxElem_Num; i++)
//    GfxElem[i].Update();
}


void Display::page_setup_statusBar() {
}
void Display::page_updateText_statusBar() {
  lcd.setTextColor(TFT_YELLOW, TFT_BLACK);
  lcd.setCursor(160, 5);
  lcd.print(ctrl.voltageBattery, 3);
  lcd.print("v");
}



void Display::page_setupStartupScreen() {
    lcd.setTextWrap(0);
    lcd.setTextSize(4);
    lcd.setTextColor(TFT_CYAN);
    lcd.setTextDatum(MC_DATUM);
    lcd.setCursor(0, 0);
    lcd.println("Drive &");
    lcd.println("BMS Reader");
    espDelay(2000);

    lcd.fillScreen(TFT_BLACK);
    lcd.setCursor(0, 0);
    lcd.setTextSize(2);
    lcd.setTextColor(TFT_DARKGREEN);
    lcd.setTextDatum(MC_DATUM);
    lcd.println("Top But: page");
    lcd.println("Bottom But: scroll");
    lcd.println();

    lcd.setTextSize(3);
    lcd.setTextColor(TFT_PURPLE);
    lcd.println("Scanning BT");


/*    lcd.drawString("LeftButton:", lcd.width() / 2, lcd.height() / 2 - 16);
    lcd.drawString("RightButton:", lcd.width() / 2, lcd.height() / 2 + 16);
    lcd.drawString("[Voltage Monitor]", lcd.width() / 2, lcd.height() / 2 + 32 );
    lcd.drawString("RightButtonLongPress:", lcd.width() / 2, lcd.height() / 2 + 48);
    lcd.drawString("[Deep Sleep]", lcd.width() / 2, lcd.height() / 2 + 64 );
    lcd.setTextDatum(TL_DATUM);
*/

}


//  Main    ////////
////////////////////
void Display::page_setupMain() {
  OnButtonChange(0, 0);
  OnButtonChange(1, 0);
  statusBar_Show = 0;

  lcd.setTextColor(TFT_GREEN, TFT_BLACK);

  lcd.setCursor(DISPLAY_MARGIN_LEFT, 0);
  lcd.println("MOSFET");
  lcd.setCursorX(DISPLAY_MARGIN_LEFT);
  lcd.println("Motor");
  lcd.setCursorX(DISPLAY_MARGIN_LEFT);
  lcd.println("Batt V");
  lcd.setCursorX(DISPLAY_MARGIN_LEFT);
  lcd.println("Batt I");
  lcd.setCursorX(DISPLAY_MARGIN_LEFT);
  lcd.println("Motor I");
  lcd.setCursorX(DISPLAY_MARGIN_LEFT);
  lcd.println("Mtr RPM");
  lcd.setCursorX(DISPLAY_MARGIN_LEFT);
  lcd.println("Duty Cyc");
  lcd.setCursorX(DISPLAY_MARGIN_LEFT);
  lcd.println("Throttle");
}
void Display::page_updateMain() {
  lcd.setTextColor(TFT_ORANGE, TFT_BLACK);

  lcd.setCursor(DISPLAY_MARGIN_VALUES, 0);
  lcd.printf("%.1f   \n", drive.tempMosfet);
  lcd.setCursorX(DISPLAY_MARGIN_VALUES);
  lcd.printf("%.1f   \n", drive.tempMotor);
  lcd.setCursorX(DISPLAY_MARGIN_VALUES);
  lcd.printf("%.1f   \n", drive.voltageBattery);
  lcd.setCursorX(DISPLAY_MARGIN_VALUES);
  lcd.printf("%.2f   \n", drive.currentBattery);

  lcd.setTextColor(TFT_CYAN, TFT_BLACK);
  lcd.setCursorX(DISPLAY_MARGIN_VALUES);
  lcd.printf("%.1f     \n", drive.currentMotor);
  lcd.setCursorX(DISPLAY_MARGIN_VALUES);
  lcd.printf("%.0f   \n", drive.rpm);
  lcd.setCursorX(DISPLAY_MARGIN_VALUES);
  lcd.printf("%.1f   \n", drive.dutyCycle*100);   // show in %
  lcd.setCursorX(DISPLAY_MARGIN_VALUES);
  lcd.printf("%.1f   \n", ctrl.throttle);
}



//  Temperatures
////////////////////
void Display::page_setupTemps() {
  lcd.setTextColor(TFT_GREEN, TFT_BLACK);

  lcd.setCursor(DISPLAY_MARGIN_LEFT, 0);
  lcd.println("VESC Temp");
  lcd.setCursorX(DISPLAY_MARGIN_LEFT);
  lcd.println("Motor Temp");
  lcd.setCursorX(DISPLAY_MARGIN_LEFT);
  lcd.println("BME Temp Dew");
  lcd.setCursorX(DISPLAY_MARGIN_LEFT);
  lcd.println("Pres Humid");
}
void Display::page_updateTemps() {
  lcd.setTextColor(TFT_RED, TFT_BLACK);

  lcd.setCursor(DISPLAY_MARGIN_VALUES, 0);
  lcd.printf("%.1f   \n", drive.tempMosfet);
  lcd.setCursorX(DISPLAY_MARGIN_VALUES);
  lcd.printf("%.1f   \n", drive.tempMotor);
  lcd.setCursorX(DISPLAY_MARGIN_VALUES);
  lcd.printf("%.1f   %.1f    \n", drive.BMETemperature, drive.BMEDewPoint);
  lcd.setCursorX(DISPLAY_MARGIN_VALUES);
  lcd.printf("%.1f   %.1f    \n", drive.BMEPressure, drive.BMEHumidity);
}


//  MCConfig
////////////////////
void Display::page_setupMCConfig() {
  lcd.setTextColor(TFT_YELLOW, TFT_BLACK);

  lcd.setCursor(DISPLAY_MARGIN_LEFT, 0);
  lcd.println("VESC Mode");
  lcd.setCursorX(DISPLAY_MARGIN_LEFT);
  lcd.println("I Limit");
  lcd.setCursorX(DISPLAY_MARGIN_LEFT);
  lcd.println("eRPM Lim");
  lcd.setCursorX(DISPLAY_MARGIN_LEFT);
  lcd.println("Motor Type");

}
void Display::page_updateMCConfig() {
//  mc_configuration driveMCConfig;
  lcd.setTextColor(TFT_GREEN, TFT_BLACK);
  
  lcd.setCursor(DISPLAY_MARGIN_LEFT, 0);
  lcd.printf("%d   \n", driveMCConfig.pwm_mode);
  lcd.setCursorX(DISPLAY_MARGIN_VALUES);
  lcd.printf("%.1f   \n", driveMCConfig.l_current_max);
  lcd.setCursorX(DISPLAY_MARGIN_VALUES);
  lcd.printf("%.1f   \n", driveMCConfig.l_max_erpm);
  lcd.setCursorX(DISPLAY_MARGIN_VALUES);
  lcd.printf("%d   \n", driveMCConfig.motor_type);

}

//  Timing
////////////////////
void Display::page_setupTiming() {
  lcd.setTextColor(TFT_PURPLE, TFT_BLACK);
  lcd.setCursor(DISPLAY_MARGIN_LEFT, 0);
  lcd.println("Timing");

  OnButtonChange(0, 0);
  OnButtonChange(1, 0);
}
void Display::page_updateTiming() {
  lcd.setTextColor(TFT_YELLOW, TFT_BLACK);
  lcd.setCursor(DISPLAY_MARGIN_LEFT, 0);
  lcd.printf("loop %d < %3d  ", perf.dtMin, perf.dtMax);
  lcd.setCursorX(DISPLAY_MARGIN_UNITS);
  lcd.println("ms");

  lcd.setTextColor(TFT_GREEN, TFT_BLACK);
  lcd.setCursor(DISPLAY_MARGIN_LEFT, lcd.getCursorY() + DISPLAY_LINESPACE);
  lcd.printf("Fast %d < %3d  ", trigFast.dtMin, trigFast.dtMax);

  lcd.setTextColor(TFT_BLUE, TFT_BLACK);
  lcd.setCursor(DISPLAY_MARGIN_LEFT, lcd.getCursorY() + DISPLAY_LINESPACE);
  lcd.printf("LCD %d < %3d  ", trigLCD.dtMin, trigLCD.dtMax);

  lcd.setTextColor(TFT_RED, TFT_BLACK);
  lcd.setCursor(DISPLAY_MARGIN_LEFT, lcd.getCursorY() + DISPLAY_LINESPACE);
  lcd.printf("Sec %d < %3d  ", trigSec.dtMin, trigSec.dtMax);

  if (butt.btn1.isPressed())
    resetPerfTimers();
}

//  Memory
////////////////////
void Display::page_setupMemory() {
    lcd.setTextColor(TFT_GREEN, TFT_BLACK);
    lcd.setCursor(DISPLAY_MARGIN_LEFT, 0);
    lcd.println("No Info");
    lcd.setCursorX(DISPLAY_MARGIN_LEFT);
    lcd.println("Yet");

    OnButtonChange(0, 0);
    OnButtonChange(1, 0);
}
void Display::page_updateMemory() {
    lcd.setTextColor(TFT_RED, TFT_BLACK);

}






void Display::OnButtonChange(int iBtnIdx, int state, int btnStates) {   // previous btnStates
  if (iBtnIdx < 0) iBtnIdx = 0;
  if (iBtnIdx > 1) iBtnIdx = 1;
  buttonInfo& b = srcnButton[iBtnIdx];
  drawButton(b, state);

  int btnStatesOther = btnStates & ~(1 << iBtnIdx);     // cancel out this button's bit
  if (state == 1) {
    if (btnStatesOther == 0)      // check for single button
      OnButtonPress(iBtnIdx);
    else               // check for double button
      OnDoubleButtonPress(iBtnIdx);
  } else {  // (state == 0)
    if (btnStates == 0)      // shouldn't happen
      ;
    else {      // check for single or double button release - if btnStates has 1 or 2 bits set
      if (btnStatesOther == 0)      // check for single button
        OnButtonRelease(iBtnIdx);
      else
        OnDoubleButtonRelease(iBtnIdx);   // must have been more than one bit set previously
    }
  }
    
}
  
void Display::OnButtonPress(int iBtnIdx) {
  log_d("OnButtonPress");
  if (buttonMode == BTNMODE_PAGE) {
    if (iBtnIdx == 0) page_changeNext();
    else if (iBtnIdx == 1) page_changePrev();
  }

}
void Display::OnButtonRelease(int iBtnIdx) {
  log_d("OnButtonRelease");

}
void Display::OnDoubleButtonPress(int iBtnIdx) {
  log_d("OnDoubleButtonPress");
  if (++buttonMode >= BTNMODE_LAST)
    buttonMode = 0;
}
void Display::OnDoubleButtonRelease(int iBtnIdx) {
  log_d("OnDoubleButtonRelease");

}

void ChangeThrottle(int dir) {
    float cng = 0.04 * dir;
    ctrl.throttle += cng;
    if (ctrl.throttle > 100) ctrl.throttle = 100;
    else if (ctrl.throttle < 0) ctrl.throttle = 0;
}
void Display::OnButtonDown(int btnStates) {
    if (buttonMode == BTNMODE_THROTTLE) {
      if (btnStates == 1)
        ChangeThrottle(1);
      else if (btnStates == 2)
        ChangeThrottle(-1);
    }
}

void Display::drawButton(buttonInfo& b, int state) {      // state 1 = pressed
    uint16_t clrDef[] = {TFT_LIGHTGREY, TFT_SILVER};
    uint16_t clr= clrDef[state];
    if (buttonMode == BTNMODE_THROTTLE)
      clr = b.clr[state];

    lcd.fillRoundRect(b.x, b.y, b.w, b.h, b.rad, clr);
    if (state)
      lcd.drawRoundRect(b.x, b.y, b.w, b.h, b.rad, TFT_DARKGREY);

    if (!b.label)
      return;
    lcd.setTextColor(TFT_LIGHTGREY);
    lcd.setTextSize(2);
    lcd.setTextDatum(MC_DATUM);
    lcd.drawString(b.label, b.x + b.w/2, b.y + b.h/2);
}



void Display::OnDeepSleepCmd() {
        int r = digitalRead(TFT_BL);
        lcd.fillScreen(TFT_BLACK);
        lcd.setTextColor(TFT_GREEN, TFT_BLACK);
        lcd.setTextDatum(MC_DATUM);
        lcd.drawString("Press again to wake up",  lcd.width() / 2, lcd.height() / 2 );
        espDelay(6000);
        digitalWrite(TFT_BL, !r);

        lcd.writecommand(TFT_DISPOFF);
        lcd.writecommand(TFT_SLPIN);
        //After using light sleep, you need to disable timer wake, because here use external IO port to wake up
        esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER);
        // esp_sleep_enable_ext1_wakeup(GPIO_SEL_35, ESP_EXT1_WAKEUP_ALL_LOW);
        esp_sleep_enable_ext0_wakeup(GPIO_NUM_35, 0);
        delay(200);
        esp_deep_sleep_start();

}
