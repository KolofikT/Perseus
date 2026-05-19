#include <Arduino.h>
#include "robotka.h"
#include "rameno.h"

void fRamenoHomePos (){
    
    rkSmartServoMove(0, 130);
    rkSmartServoMove(1, 205);
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
    if (rkButtonIsPressed(BTN_OFF))     { 
        //fRamenoOwn();
        // SCÉNÁŘ PRO MANIPULÁTOR 1 (Nyní má ID 0)
        //MoveGrabber(0, 100);                // 1. Otevři chapadlo
        MoveManipulator(0, 180, 50, 0);     // 2. Dojeď pro předmět před sebe                       | Cíl: X=180, Y=50, Základna = 0° (vpřed)
        delay(10000);
        //MoveGrabber(0, 150);                // 3. Zavři chapadlo (chytni předmět)
        MoveManipulator(0, 180, 150, -45);  // 4. Zvedni ho do výšky a otoč se s ním doprava        | Cíl: X=180, Y=50, Základna = -90° (vpravo)
        //MoveGrabber(0, 100);                // 5. Otevři chapadlo (pusť předmět)
        delay(10000);
        printf("\n");
        
    }
    
    //rkSmartServosPosicion(0);
    //rkSmartServosPosicion(1);
    //rkSmartServosPosicion(2);
    delay(10);
}

/*
int main() {
    // SCÉNÁŘ PRO MANIPULÁTOR 1 (Nyní má ID 0)
    //MoveGrabber(0, 100);                // 1. Otevři chapadlo
    MoveManipulator(0, 180, 50, 0);     // 2. Dojeď pro předmět před sebe                       | Cíl: X=180, Y=50, Základna = 0° (vpřed)
    //MoveGrabber(0, 150);                // 3. Zavři chapadlo (chytni předmět)
    MoveManipulator(0, 180, 150, -45);  // 4. Zvedni ho do výšky a otoč se s ním doprava        | Cíl: X=180, Y=50, Základna = -90° (vpravo)
    //MoveGrabber(0, 100);                // 5. Otevři chapadlo (pusť předmět)

    printf("\n");
    
    // // SCÉNÁŘ PRO MANIPULÁTOR 2 (Nyní má ID 1)
    //MoveGrabber(1, 100);                
    //MoveManipulator(1, 180, 50, 0);     // Zde se ukáže využití ID 4, 5, 6, 7 a reverzního otáčení
    //MoveGrabber(1, 150);

    return 0;
}
    */