#include <iostream>
#include <cmath>
#include <algorithm>


using namespace std;

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

///////////////////////////////////////////////////////////////////////////////////////////

//Smart servo 0 (Otaceni zakladny 1. manipulátoru)  - min  (right) - 45°   mid (front) - 135°   max (left)  - 225°      rozsah: 180°
//Smart servo 1 (Pohyb 1. ramene 1. manipulátoru)   - min  (down)  - 140°                       max (up)    - 235°      rozsah: 95°
//Smart servo 2 (Pohyb 2. ramene 1. manipulátoru)   - min  (down)  -  0°                        max (up)  - 85°         rozsah: 85°
//Smart servo 3 (Chytání grabberem 1. manipulátoru) - min  (right) -       mid () - 150°        max (left)  -           rozsah:
//Základní pozice 1. manipulátoru:  |Základna - 135°|1. Rameno -  200°|2. Rameno - 85°|Grabber - x°|

//Rozlozeni z = 0:      1. Rameno - 147°    2. Rameno - 14° 
//Rozlozeni z = 55:     1. Rameno - 160°    2. Rameno - 5°

//Slozeni z = 0:        1. Rameno - 200°    2. Rameno - 85°
//Slozeni z = 38:       1. Rameno - 235°    2. Rameno - 55°


//////////////////////////////////////////////////////////////////////////////////////////

// ==============================================================================
// TUTO FUNKCI SMAŽ, AŽ TO BUDEŠ NAHRÁVAT DO ROBOTA 
void rkSmartServoMove(int cislo_serva, int uhel) {
    cout << "  -> [HW] Fyzicky presouvam servo ID " << cislo_serva << " na uhel " << uhel << "°" << endl;
}
// ==============================================================================

/*
  HLAVNÍ OVLÁDACÍ FUNKCE MANIPULÁTORU
  iManipulator_ID          : 0 nebo 1 (určuje, který manipulátor se hýbe a jaké má rozměry nástroje)
  iTarget_Tip_Absolute_X   : Cílová pozice X (vpřed)
  iTarget_Tip_Absolute_Y   : Cílová pozice Y (výška od země)
  iBase_Angle              : Úhel natočení základny (-90° vpravo, 0° vpřed, +90° vlevo)
*/
void MoveManipulator(int iManipulator_ID, int iTarget_Tip_Absolute_X, int iTarget_Tip_Absolute_Y, int iBase_Angle) {
    
    // VALIDACE ZÁKLADNY
    if (iBase_Angle < -90 || iBase_Angle > 90) {
        cout << "[CHYBA M" << iManipulator_ID << "] Úhel otáčení základny (" << iBase_Angle << "°) je mimo limit -90° až +90°!" << endl;
        return;
    }

    // -- ROZMĚRY MANIPULÁTORU
    // DÉLKY RAMEN
    int iL1 = 115;                  
    int iL2 = 145;                  
    
    // POSUNUTÍ RAMENE
    int iShoulder_Offset_X = -20;   
    int iShoulder_Offset_Y = 90;    

    // PUSUNUTÍ NÁDSTAVCE
    int iTool_Offset_X = 0;
    int iTool_Offset_Y = 0;
    int iServo0_Base = 0;

    // ROZMĚRY MANUPULÁTORU DLE ID
    if (iManipulator_ID == 0) {
        
        // MANIPULÁTOR 1
        iTool_Offset_X = 37;        
        iTool_Offset_Y = -54;       // -41 - 13
        iServo0_Base = 135 + iBase_Angle; // Normální otáčení
       
    } else if (iManipulator_ID == 1) {
        
        // MANIPULÁTOR 2
        iTool_Offset_X = 37;
        iTool_Offset_Y = -54;       
        iServo0_Base = 135 - iBase_Angle; // REVERZACE OTÁČENÍ (kvůli obrácené montáži)
        
    } else {
        cout << "[CHYBA] Neznámé ID manipulátoru! Zadej 1 nebo 2." << endl;
        return;
    }

    // -- PŘEPOČET SOUŘADNIC
    // ABSOLUTNÍ SOUŘADNICE (K RAMENU)
    int iWrist_Absolute_X = iTarget_Tip_Absolute_X - iTool_Offset_X;
    int iWrist_Absolute_Y = iTarget_Tip_Absolute_Y - iTool_Offset_Y; 

    // SOUŘADNICE (K OSE OTÁČENÍ MANIPULÁTORU)
    int iRelative_X = iWrist_Absolute_X - iShoulder_Offset_X;
    int iRelative_Y = iWrist_Absolute_Y - iShoulder_Offset_Y;

    // -- INVERZNÍ KINEMATIKA
    float rX = iRelative_X;
    float rY = iRelative_Y; 
    float rL1 = iL1;
    float rL2 = iL2;

    float rDistance_squared = rX * rX + rY * rY;
    float rDistance = sqrt(rDistance_squared);

    if (rDistance > (rL1 + rL2) || rDistance < abs(rL1 - rL2)) { 
        cout << "[CHYBA M" << iManipulator_ID << "] Cílový bod (X:" << iTarget_Tip_Absolute_X << ", Y:" << iTarget_Tip_Absolute_Y 
             << ") je absolutně mimo fyzický dosah ramen!" << endl;
        return; 
    }

    float rCos_Theta2 = max(-1.0f, min(1.0f, (rDistance_squared - rL1 * rL1 - rL2 * rL2) / (2.0f * rL1 * rL2)));
    
    float rTheta2_rad = acos(rCos_Theta2);
    float alpha = atan2(rY, rX);
    float beta = atan2(rL2 * sin(rTheta2_rad), rL1 + rL2 * cos(rTheta2_rad));
    
    float rTheta1_rad = alpha + beta;

    // VÝPOČET UHLŮ
    int iDeg1 = round(rTheta1_rad * (180.0f / M_PI));
    int iDeg2 = round(rTheta2_rad * (180.0f / M_PI));

    // PŘEPOČET PRO KONKRÉTNÍ SERVA
    int iServo1_Arm1 = iDeg1 + 140;      
    int iServo2_Arm2 = iDeg2 - iDeg1;    

    // VALIDACE LIMITŮ A FYZICKÝ POHYB
    if (iServo1_Arm1 >= 140 && iServo1_Arm1 <= 235 &&
        iServo2_Arm2 >= 0 && iServo2_Arm2 <= 85) {
        
        cout << "[OK M" << iManipulator_ID << "] Přesouvám robota na bod (X:" << iTarget_Tip_Absolute_X 
             << ", Y:" << iTarget_Tip_Absolute_Y << ") s natočením " << iBase_Angle << "°..." << endl;
        
        int iServo_ID_Offset = iManipulator_ID * 4; 
        
        rkSmartServoMove(0 + iServo_ID_Offset, iServo0_Base); 
        rkSmartServoMove(1 + iServo_ID_Offset, iServo1_Arm1); 
        rkSmartServoMove(2 + iServo_ID_Offset, iServo2_Arm2); 
        
    } else {
        cout << "[CHYBA M" << iManipulator_ID << "] Bod (X:" << iTarget_Tip_Absolute_X << ", Y:" << iTarget_Tip_Absolute_Y 
             << ") překračuje povolené limity serv! (R1: " << iServo1_Arm1 << "°, R2: " << iServo2_Arm2 << "°)" << endl;
    }
}

// ==============================================================================

/*
  FUNKCE PRO OVLÁDÁNÍ CHAPADLA
  iManipulator_ID   : 0 nebo 1
  iPoloha_Grabberu  : OPEN / CLOSE / ...
*/
void MoveGrabber(int iManipulator_ID, int iPoloha_Grabberu) {
    
    // OVĚŘENÍ ID MANIPULÁTORU
    if (iManipulator_ID != 0 && iManipulator_ID != 1) {
        cout << "[CHYBA] Neznámé ID manipulátoru!" << endl;
        return;
    }

    // POSUNUTÍ ID SERVA PODLE ID MANIPULÁTORU
    int iServo_ID_Offset = iManipulator_ID * 4; 
    
    // POHYB GRABBEREM
    rkSmartServoMove(3 + iServo_ID_Offset, iPoloha_Grabberu);
    
    cout << "[OK M" << iManipulator_ID << "] Chapadlo nastaveno na " << iPoloha_Grabberu << "°" << endl;
}

// ==============================================================================
// ==============================================================================

int main() {
    // SCÉNÁŘ PRO MANIPULÁTOR 1
    MoveGrabber(1, 100);                // 1. Otevři chapadlo
    MoveManipulator(1, 180, 50, 0);     // 2. Dojeď pro předmět před sebe                       | Cíl: X=180, Y=50, Základna = 0° (vpřed)
    MoveGrabber(1, 150);                // 3. Zavři chapadlo (chytni předmět)
    MoveManipulator(1, 180, 150, -90);  // 4. Zvedni ho do výšky a otoč se s ním doprava        | Cíl: X=180, Y=50, Základna = -90° (vpravo)
    MoveGrabber(1, 100);                // 5. Otevři chapadlo (pusť předmět)

    return 0;
}