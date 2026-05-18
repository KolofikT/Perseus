#include <iostream>
#include <cmath>
#include <algorithm>


using namespace std;

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

///////////////////////////////////////////////////////////////////////////////////////////

//Smart servo 0 (Otaceni zakladny 1. manipulátoru)  - min  (right)  - 50°      mid (front)    - 135°      max (left)  - 225°    
//Smart servo 1 (Pohyb 1. ramene 1. manipulátoru)   - min  (down)   - 140°                              max (up)    - 235°
//Smart servo 2 (Pohyb 2. ramene 1. manipulátoru)   - min  (down)  -  0°                               max (up)  - 85°   
//Smart servo 3 (Chytání grabberem 1. manipulátoru) - min  (right)  -        mid (front)    - 150°      max (left)  -    
//Základní pozice 1. manipulátoru: Základna - 135° 1. Rameno -  200°  2. Rameno - 85°    Grabber - x°

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

    //float rCos_Theta2 = (rDistance_squared - rL1 * rL1 - rL2 * rL2) / (2.0f * rL1 * rL2);
    float rCos_Theta2 = max(-1.0f, min(1.0f, (rDistance_squared - rL1 * rL1 - rL2 * rL2) / (2.0f * rL1 * rL2)));
    
    float rTheta2_rad = acos(rCos_Theta2);
    float alpha = atan2(rY, rX);
    float beta = atan2(rL2 * sin(rTheta2_rad), rL1 + rL2 * cos(rTheta2_rad));
    
    float rTheta1_rad = alpha + beta;

    int deg1 = round(rTheta1_rad * (180.0f / M_PI));
    int deg2 = round(rTheta2_rad * (180.0f / M_PI));

    sResult.iTheta1 = deg1;
    sResult.iTheta2 = deg2;

    if (sResult.iTheta1 >= 0 && sResult.iTheta1 <= 90 &&
        sResult.iTheta2 >= 0 && sResult.iTheta2 <= 90) {
        sResult.bIsValid = true;
    }

    return sResult;
}

int main() {
    int iL1 = 100;
    int iL2 = 100;
    
    int iTarget_x = 150;
    int iTarget_y = 50;

    sKinematicResult angles = CalculateAngles(iTarget_x, iTarget_y, iL1, iL2);

    if (angles.bIsValid) {
        // Už nepotřebujeme std::cout
        cout << "Úhel ramene 1: " << angles.iTheta1 << "\n";
        cout << "Úhel ramene 2: " << angles.iTheta2 << "\n";
    } else {
        cout << "Chyba: Mimo dosah nebo limit uhlu.\n";
    }

    return 0;
}