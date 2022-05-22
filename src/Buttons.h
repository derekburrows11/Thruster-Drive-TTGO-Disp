#pragma once

#include <Button2.h>

 
class buttons {

//    static Button2 btn1;
//    static Button2 btn2;
public:
    Button2 btn1;
    Button2 btn2;

    int btnStates;      // bitwise current state of buttons

public:
//    buttons();
    void init();
    void loop();

    static void OnPress(Button2& b);
    static void OnRelease(Button2& b);
//    void OnPress(Button2& b);
//    void OnRelease(Button2& b);

};


