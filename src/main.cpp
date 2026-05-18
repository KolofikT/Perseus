#include <Arduino.h>
#include "robotka.h"

void RamenoHomePos (){
    
    rkSmartServoMove(0, 130);
    rkSmartServoMove(1, 190);
    rkSmartServoMove(2, 80);

}

void RamenoMaxCompose (){
    
    rkSmartServoMove(0, 130);
    rkSmartServoMove(1, 215);
    rkSmartServoMove(2, 80);

}

void RamenoMaxDecompose (){
    
    rkSmartServoMove(0, 130);
    rkSmartServoMove(1, 190);
    rkSmartServoMove(2, 80);

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
    if (rkButtonIsPressed(BTN_UP)) {rkSmartServoMove(1, rkSmartServosPosicion(1) + 1); }
    if (rkButtonIsPressed(BTN_DOWN)) { rkSmartServoMove(1, rkSmartServosPosicion(1) - 1 ); } 
    if (rkButtonIsPressed(BTN_LEFT)) { rkSmartServoMove(1, rkSmartServosPosicion(2) - 1 ); }
    if (rkButtonIsPressed(BTN_RIGHT)) { rkSmartServoMove(1, rkSmartServosPosicion(2) + 1 );}
    if (rkButtonIsPressed(BTN_ON)) { RamenoHomePos(); }
    
    //rkSmartServosPosicion(0);
    //rkSmartServosPosicion(1);
    //rkSmartServosPosicion(2);
    delay(10);
}

