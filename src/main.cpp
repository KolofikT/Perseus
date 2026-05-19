#include <Arduino.h>
#include "robotka.h"

void fRamenoHomePos (){
    
    rkSmartServoMove(0, 130);
    rkSmartServoMove(1, 190);
    rkSmartServoMove(2, 80);

}

void fRamenoMaxCompose (){
    
    rkSmartServoMove(0, 130);
    rkSmartServoMove(1, 215);
    rkSmartServoMove(2, 80);

}

void fRamenoMaxDecompose (){
    
    rkSmartServoMove(0, 130);
    rkSmartServoMove(1, 190);
    rkSmartServoMove(2, 80);

}

void fRamenoOwn (){
    
    rkSmartServoMove(0, 135);
    rkSmartServoMove(1, 203);
    rkSmartServoMove(2, 37);

}

void setup() {
    rkConfig cfg;
    rkSetup(cfg);
    printf("Robotka started!\n");
    Serial.begin(115200);
    
    rkLedRed(true); // Turn on red LED
    rkLedBlue(true); // Turn on blue LED


    rkSmartServoInit(0, 0, 240, 500, 3);
    rkSmartServoInit(1, 140, 235);
    rkSmartServoInit(2, 0, 240);
    
    
}
void loop() {
    if (rkButtonIsPressed(BTN_UP))      { rkSmartServoMove(1, rkSmartServosPosicion(1) + 5); }
    if (rkButtonIsPressed(BTN_DOWN))    { rkSmartServoMove(1, rkSmartServosPosicion(1) - 5 ); } 
    if (rkButtonIsPressed(BTN_LEFT))    { rkSmartServoMove(0, rkSmartServosPosicion(0) - 5 ); }
    if (rkButtonIsPressed(BTN_RIGHT))   { rkSmartServoMove(0, rkSmartServosPosicion(0) + 5 );}
    if (rkButtonIsPressed(BTN_ON))      { fRamenoHomePos(); }
    if (rkButtonIsPressed(BTN_OFF))     { fRamenoOwn(); }
    
    //rkSmartServosPosicion(0);
    //rkSmartServosPosicion(1);
    //rkSmartServosPosicion(2);
    delay(10);
}

