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
    int iMathTheta1; 
    int iMathTheta2; 
    int iServo1;
    int iServo2;
    bool bIsValid; 
};

sKinematicResult CalculateAngles(int x, int y, int L1, int L2) {
    sKinematicResult sResult = {0, 0, 0, 0, false};

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

    int iDeg1 = round(rTheta1_rad * (180.0f / M_PI));
    int iDeg2 = round(rTheta2_rad * (180.0f / M_PI));

    sResult.iMathTheta1 = iDeg1;
    sResult.iMathTheta2 = iDeg2;

    
    sResult.iServo1 = iDeg1 + 140;              // Servo 1: 140 je vodorovně, zvedá se nahoru
                                                // Servo 2: 0 je vodorovně, roste směrem dolů.          
    sResult.iServo2 = iDeg2 - iDeg1;            // Fyzický absolutní úhel spočítáme z rozdílu relativních úhlů ramen.

    // Validace limitů serv
        // Servo 1 může od 140 do 235
        // Servo 2 může od 0 (vodorovně) do 85 (téměř kolmo dolů)
    if (sResult.iServo1 >= 140 && sResult.iServo1 <= 235 &&
        sResult.iServo2 >= 0 && sResult.iServo2 <= 85) {
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
    int iTool_Offset_X = 37;        // Nástavec trčí 37 mm dopředu od zápěstí
    int iTool_Offset_Y = -41-13;    // Nástavec směřuje 41 mm dolů od zápěstí

    // Absolutní cíl (vztažen ke středu osy otáčení základny a k zemi)
    int iTarget_Tip_Absolute_X = 180;
    int iTarget_Tip_Absolute_Y = 50;  

    // --- PŘEPOČTY SOUŘADNIC ---
    // 1. Wrist v absolutních souřadnicích
    int iWrist_Absolute_X = iTarget_Tip_Absolute_X - iTool_Offset_X;
    int iWrist_Absolute_Y = iTarget_Tip_Absolute_Y - iTool_Offset_Y; 

    // 2. Relativní vzdálenost Wrist od prvního kloubu (lokální souřadnice pro matematiku)
    int iRelative_X = iWrist_Absolute_X - iShoulder_Offset_X;
    int iRelative_Y = iWrist_Absolute_Y - iShoulder_Offset_Y;

    // Do funkce už jen čistoá vzdálenost, kterou musí ramena překonat
    sKinematicResult angles = CalculateAngles(iRelative_X, iRelative_Y, iL1, iL2);

    //Vypsání cílových souřadnic
    cout << "--- CÍLOVÉ SOUŘADNICE ZÁPĚSTÍ ---\n";
    cout << "Relativní X: " << iRelative_X << ", Relativní Y: " << iRelative_Y << "\n\n";

    if (angles.bIsValid) {
        
        cout << "--- MATEMATIKA ---\n";
        cout << "Úhel ramene 1 (od 0): " << angles.iMathTheta1 << "°\n";
        cout << "Úhel ramene 2 (vnitřní ohyb): " << angles.iMathTheta2 << "°\n\n";
    

        cout << "--- HODNOTY PRO SERVA ---\n";
        cout << "Smart servo 1: " << angles.iServo1 << "°\n";
        cout << "Smart servo 2: " << angles.iServo2 << "°\n";
    } else {
        cout << ">>> CHYBA: Bod je nedosažitelný nebo překračuje limity serv! <<<\n";
        cout << "Spočítáno by bylo -> Servo 1: " << angles.iServo1 << "°, Servo 2: " << angles.iServo2 << "°\n";
        cout << "Zkontroluj, zda by Servo 2 nemuselo jít pod 0° (zvednout se nad rovinu) nebo nad 85°.\n";
    }

    return 0;
}