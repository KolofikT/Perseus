#include <iostream>
#include <cmath>
#include <algorithm>


using namespace std;

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

///////////////////////////////////////////////////////////////////////////////////////////

//Smart servo 0 (Otaceni zakladny 1. manipulátoru)  - min  (right) - 50°   mid (front) - 135°   max (left)  - 225°      rozsah: 180°
//Smart servo 1 (Pohyb 1. ramene 1. manipulátoru)   - min  (down)  - 140°                       max (up)    - 235°      rozsah: 95°
//Smart servo 2 (Pohyb 2. ramene 1. manipulátoru)   - min  (down)  -  0°                        max (up)  - 85°         rozsah: 85°
//Smart servo 3 (Chytání grabberem 1. manipulátoru) - min  (right) -       mid () - 150°        max (left)  -           rozsah:
//Základní pozice 1. manipulátoru:  |Základna - 135°|1. Rameno -  200°|2. Rameno - 85°|Grabber - x°|

//Rozlozeni z = 0:      1. Rameno - 147°    2. Rameno - 14° 
//Rozlozeni z = 55:     1. Rameno - 160°    2. Rameno - 5°

//Slozeni z = 0:        1. Rameno - 200°    2. Rameno - 85°
//Slozeni z = 38:       1. Rameno - 235°    2. Rameno - 55°


//////////////////////////////////////////////////////////////////////////////////////////

struct sKinematicResult {
    int iTheta1; 
    int iTheta2; 
    bool bIsValid;  
};

sKinematicResult CalculateAngles(int x, int y, int L1, int L2) {
    sKinematicResult sResult = {0, 0, false};

    float rX = x;
    float rY = y; 
    
    float rL1 = L1;
    float rL2 = L2;

    float rDistance_squared = rX * rX + rY * rY;
    float rDistance = sqrt(rDistance_squared);

    if (rDistance > (rL1 + rL2) || rDistance < abs(rL1 - rL2)) { return sResult; }

    float rCos_Theta2 = max(-1.0f, min(1.0f, (rDistance_squared - rL1 * rL1 - rL2 * rL2) / (2.0f * rL1 * rL2)));
    
    float rTheta2_rad = acos(rCos_Theta2);
    float alpha = atan2(rY, rX);
    float beta = atan2(rL2 * sin(rTheta2_rad), rL1 + rL2 * cos(rTheta2_rad));
    
    float rTheta1_rad = alpha + beta;

    int deg1 = round(rTheta1_rad * (180.0f / M_PI));
    int deg2 = round(rTheta2_rad * (180.0f / M_PI));

    sResult.iTheta1 = deg1;
    sResult.iTheta2 = deg2;

    if (sResult.iTheta1 >= 0 && sResult.iTheta1 <= 95 &&
        sResult.iTheta2 >= 0 && sResult.iTheta2 <= 85) {
        sResult.bIsValid = true;
    }

    return sResult;
}

int main() {

    // Délky ramen
    int iL1 = 115;                  // Délka 1. Ramene
    int iL2 = 145;                  // Délka 2. Ramene
    
    // Posunutí základny
    int iShoulder_Offset_X = -20;   // Rameno je 20 mm vzad od osy otáčení
    int iShoulder_Offset_Y = 90;    // Rameno je 90 mm nad zemí

    // Posunutí toolu
    int iTool_Offset_X = 35;        // Nástavec trčí 35 mm dopředu od zápěstí
    int iTool_Offset_Y = -45;       // Nástavec směřuje 45 mm dolů od zápěstí

    // Absolutní cíl (vztažen ke středu osy otáčení základny a k zemi)
    int iTarget_Tip_Absolute_X = 220;
    int iTarget_Tip_Absolute_Y = 60;  

    // --- PŘEPOČTY SOUŘADNIC ---
    // 1. Wrist v absolutních souřadnicích
    int iWrist_Absolute_X = iTarget_Tip_Absolute_X - iTool_Offset_X;
    int iWrist_Absolute_Y = iTarget_Tip_Absolute_Y - iTool_Offset_Y; 

    // 2. Relativní vzdálenost Wrist od prvního kloubu (lokální souřadnice pro matematiku)
    int iRelative_X = iWrist_Absolute_X - iShoulder_Offset_X;
    int iRelative_Y = iWrist_Absolute_Y - iShoulder_Offset_Y;

    // Do funkce už jen čistoá vzdálenost, kterou musí ramena překonat
    sKinematicResult angles = CalculateAngles(iRelative_X, iRelative_Y, iL1, iL2);

    // Vypíšeme si matematiku VŽDY, abychom věděli, co to vlastně spočítalo
    cout << "--- MATEMATIKA ---\n";
    cout << "Vypočítaný úhel ramene 1: " << angles.iTheta1 << "° (Limit: 0-95°)\n";
    cout << "Vypočítaný úhel ramene 2: " << angles.iTheta2 << "° (Limit: 0-85°)\n\n";

    if (angles.bIsValid) {
        
        //Přepočet na reálné hodnoty pro serva
        int iServo1_Angle = angles.iTheta1 + 140;   //Servo 1 začíná fyzicky až na 140°
        int iServo2_Angle = angles.iTheta2;         //  Servo 2 začíná na 0°, takže se rovná výpočtu  

        cout << "--- HODNOTY PRO SERVA ---\n";
        cout << "Smart servo 1: " << iServo1_Angle << "°\n";
        cout << "Smart servo 2: " << iServo2_Angle << "°\n";
    } else {
        cout << "Chyba: Mimo dosah nebo byl překročen limit úhlů (95° pro R1, 85° pro R2).\n";
    }

    return 0;
}