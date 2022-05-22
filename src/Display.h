#pragma once

#include <inttypes.h>

struct buttonInfo {
    int x, y, w, h, rad;
    const char* label;
    uint16_t clr[2];    // normal and pressed colors
};


enum DISPLAY_PAGE_t {
  PAGE_MAIN = 0,
  PAGE_TEMPS,
  PAGE_MCCONFIG,
  PAGE_TIMING,
  PAGE_MEMORY,
  PAGE_LAST,

  PAGE_DRIVEFB,
  PAGE_CTRLPOWER,


  PAGE_SUB0 = 0,
  PAGE_SUB1 = 20,
  PAGE_SUB2 = PAGE_SUB1 * 2,
  PAGE_SUB3 = PAGE_SUB1 * 3,
};

enum BUTTON_MODE_t {
  BTNMODE_THROTTLE = 0,
  BTNMODE_PAGE,
  BTNMODE_PARAM,
  BTNMODE_LAST,
};

class Display {

  int buttonMode = BTNMODE_THROTTLE;

  //DISPLAY_PAGE_t pageCurr = PAGE_MAIN;
  int pageCurr = PAGE_MAIN;
  int pageCurrSub = 0;
  int pageSubs[PAGE_LAST];

  bool statusBar_Show;
  int updateTextCycle = 0;    // staged text updating
  int updateTextMax = 4;      // number of graphics updates for one text update
  bool updateTextNow;
  int lineNum;

  static const int GfxElem_Max = 20;
  int GfxElem_Num = 0;
//  GFX_ELEM GfxElem[GfxElem_Max];
  int msgNum = 0;

public:
  void init();
  void update();

  void startupScreen();
  void page_change(int page);
  void page_changeNext();
  void page_changePrev();
  void page_changeSubNext();
  void page_changeSubPrev();


protected:
  void pushSprite();
  void fill(uint16_t clr);


  void page_setup();
  void page_update();

  void page_setup_statusBar();
//    void page_update_statusBar();
  void page_updateText_statusBar();


  // Pages ////////
  ////////////////
  void page_setupStartupScreen();

  void page_setupMain();
  void page_updateMain();

  void page_setupTemps();
  void page_updateTemps();

  void page_setupMCConfig();
  void page_updateMCConfig();

  void page_setupTiming();
  void page_updateTiming();

  void page_setupMemory();
  void page_updateMemory();


public:
  void OnButtonChange(int iBtnIdx, int state, int btnStates = 0);
  // bitwise button states
  void OnButtonDown(int btnStates);

protected:
  void OnButtonPress(int iBtnIdx);
  void OnButtonRelease(int iBtnIdx);
  void OnDoubleButtonPress(int iBtnIdx);
  void OnDoubleButtonRelease(int iBtnIdx);
  void drawButton(buttonInfo& b, int state);

  void OnDeepSleepCmd();
};

