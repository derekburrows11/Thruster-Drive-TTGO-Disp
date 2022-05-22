
#include "config.h"
#include "Display.h"
#include "Buttons.h"






void buttons::OnPress(Button2& b) {
    int iBtnIdx = 0;
    if (b == butt.btn2)
        iBtnIdx = 1;
    disp.OnButtonChange(iBtnIdx, 1, butt.btnStates);
}
void buttons::OnRelease(Button2& b) {
    int iBtnIdx = 0;
    if (b == butt.btn2)
        iBtnIdx = 1;
    disp.OnButtonChange(iBtnIdx, 0, butt.btnStates);
}
/*void OnLongClick(Button2& b) {
    disp.OnDeepSleepCmd();
}*/

//Button2 buttons::btn1(PIN_BUTTON1);
//Button2 buttons::btn2(PIN_BUTTON2);

//buttons::buttons() : btn1(PIN_BUTTON1), btn2(PIN_BUTTON2) {}

void buttons::init() {
    btn1.begin(PIN_BUTTON1);
    btn2.begin(PIN_BUTTON2);

    btn1.setPressedHandler(OnPress);
    btn2.setPressedHandler(OnPress);
    btn1.setReleasedHandler(OnRelease);
    btn2.setReleasedHandler(OnRelease);
//    btn1.set(OnPress);

    btn1.setLongClickHandler([](Button2& b) {
//        disp.OnDeepSleepCmd();
    });
 }

void buttons::loop() {
    btn1.loop();
    if (btn1.isPressed()) btnStates |=  0x01;      // update btnStates after checking each button or double button press could be missed
    else                  btnStates &= ~0x01;

    btn2.loop();
    if (btn2.isPressed()) btnStates |=  0x02;      // update btnStates after checking each button or double button press could be missed
    else                  btnStates &= ~0x02;

    if (btnStates) {
        btnStates &= 0x03;      // check all other bits are clear
        disp.OnButtonDown(btnStates);
    }
}

