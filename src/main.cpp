#include <Arduino.h>
#include "robotka.h"
#include "rameno.h"

void fRamenoHomePos (){
    
    rkSmartServoMove(0, 130);
    rkSmartServoMove(1, 225);
    rkSmartServoMove(2, 60);

    //fMoveManipulator(0, 130, 65, 0);

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


    // Nastavení Chytrých serv
    rkSmartServoInit(0, 45, 2250, 500, 3);
    rkSmartServoInit(1, 140, 235);
    rkSmartServoInit(2, 0, 90);
    rkSmartServoInit(3, 0, 120);
    
    
}
void loop() {
    if (rkButtonIsPressed(BTN_LEFT) && rkButtonIsPressed(BTN_RIGHT))    { fMoveGrabber(0, iOpen_Grabber_RA); }
    if (rkButtonIsPressed(BTN_UP) && rkButtonIsPressed(BTN_DOWN))       { fMoveGrabber(0, iClose_Grabber_TC); }
    // if (rkButtonIsPressed(BTN_UP))      { rkSmartServoMove(1, rkSmartServosPosicion(1) + 5); }
    // if (rkButtonIsPressed(BTN_DOWN))    { rkSmartServoMove(1, rkSmartServosPosicion(1) - 5 ); } 
    // if (rkButtonIsPressed(BTN_LEFT))    { rkSmartServoMove(2, rkSmartServosPosicion(2) + 5); }
    // if (rkButtonIsPressed(BTN_RIGHT))   { rkSmartServoMove(2, rkSmartServosPosicion(2) - 5);}
    if (rkButtonIsPressed(BTN_ON))      { fRamenoHomePos(); }
    if (rkButtonIsPressed(BTN_OFF))     { 
        
        // SCÉNÁŘ PRO MANIPULÁTOR 1 (ID 0)
        
        fMoveGrabber(0, iOpen_Grabber_RA);      // 1. Otevři chapadlo
        delay(1500);
        
        fMoveManipulator(0, 180, 65, 0);        // 2. Dojeď pro předmět před sebe                       | Cíl: X=180, Y=50, Základna = 0° (vpřed)
        delay(10000);
        
        fMoveGrabber(0, iClose_Grabber_RA);     // 3. Zavři chapadlo (chytni předmět)
        delay(1500);    
        
        fMoveManipulator(0, 180, 150, -45);     // 4. Zvedni ho do výšky a otoč se s ním doprava        | Cíl: X=180, Y=50, Základna = -90° (vpravo)
        delay(10000);
        
        fMoveGrabber(0, iOpen_Grabber_RA);      // 5. Otevři chapadlo (pusť předmět)
        delay(1500);

        printf("\n");
        
    }
    delay(10);
}