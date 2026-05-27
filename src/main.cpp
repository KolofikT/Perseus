#include "robotka.h"
#include <Arduino.h>
#include <string>

#ifdef USE_VL53L0X
#include <Adafruit_VL53L0X.h>
#endif
#include <Adafruit_TCS34725.h>

// Globální instance pro senzory a časovač výpisů
Adafruit_VL53L0X laser;
Adafruit_TCS34725 rgbSensor;
uint32_t lastSensorPrint = 0;

// Include ovládacích souborů pro Persea
#include "Manipulator.h"
#include "Roadside.h"
#include "Movement.h"
#include "ContestTimer.h"


// Nastavení Roadsidu
GameManager RoadsideGame;

// Nastavení Časovače
ContestTimer GameTimer; 

// Proměnné pro MENU
enum class MenuState {
    SELECT_CONTEST,
    ROADSIDE_SELECT_TEAM,
    ROADSIDE_SELECT_LAYOUT,
    ROADSIDE_WAIT_START,
    TOYCLEANUP_WAIT_START,
    GAME_RUNNING
};

// Proměnné pro Soutěž
enum class Contest { None, Roadside, ToyCleanUp };

// Status startovního MENU
MenuState eCurrentState = MenuState::SELECT_CONTEST;
Contest eSelectedContest = Contest::None;
TeamColor eSelectedTeam = TeamColor::Blue; // Výchozí tým
int iSelectedLayout = 0; 
bool bRoadsideGameStarted = false;

// Místo Startu  robota
float rCurrentRobotX = 1400.0f; 
float rCurrentRobotY = 200.0f;  

void setup() {

    //rkLedYellow(true);
    
    // -- Start SerialMonitoru
    Serial.begin(115200);
    delay(1000); 

    // -- Konfugurace robotky
    rkConfig cfg;
    
    cfg.pocet_chytrych_serv = 7;
    delay(500);
    
    rkSetup(cfg);
    delay(200);

    printf("ROBOTKA INIT COMPLETE\n");

    // -- Inicializace I2C senzorů

    // Barevný senzor
    Wire.begin(14, 26, 400000);  // SDA = 14, SCL = 26
    Wire.setTimeOut(1);
    
    // Laserový senzor
    Wire1.begin(21, 22, 400000); // SDA = 21, SCL = 22
    Wire1.setTimeOut(1);

    printf("I2C INIT ....\n");
    
    // Init Laserového senzoru
    rk_laser_init("laser", Wire1, laser, 27, 0x30);
    
    // Init Barevného senzoru
    rkColorSensorInit("color", Wire, rgbSensor);
    
    printf("I2C INIT COMPLETE.\n");

    // -- Inicializace Chytrých serv

    printf("CHYTRA SERVA INIT ....\n");

    // Manipulator ID 0:
    rkSmartServoInit(0, 45, 225, 500, 3); 
    rkSmartServoInit(1, 140, 235);
    rkSmartServoInit(2, 0, 90);
    rkSmartServoInit(3, 0, 120);
    
    // // Manipulator ID 1:
    // rkSmartServoInit(4, 0, 240, 500, 3); 
    // rkSmartServoInit(5, 0, 240);
    // rkSmartServoInit(6, 0, 240);
    
    printf("CHYTRA SERVA INIT COMPLETE\n");

    printf(" ---- SETUP COMPLETE ----\n");
    printf("Waiting for start...\n");


    //rkWaitForStart(); 

    printf(" ---- START ----\n");
}
void loop() {

    // switch (eCurrentState) {
        
    //     // ========================================================
    //     // MENU 0: Výběr soutěže (Contest)
    //     // ========================================================

    //     case MenuState::SELECT_CONTEST:
            
    //         // Signalizace: Modrá pro Roadside, Zelená pro ToyCleanUp, Žlutá pro NIC
    //         rkLedAll(false);
    //         if (eSelectedContest == Contest::Roadside) rkLedBlue(true);
    //         else if (eSelectedContest == Contest::ToyCleanUp) rkLedGreen(true);
    //         else rkLedYellow(true);
            
    //         // Výběr a uložení volby pomocí BTN_1 a BTN_2
    //         if (rkButton1(true)) { eSelectedContest = Contest::Roadside; printf("Vybrano: Roadside\n"); }
    //         if (rkButton2(true)) { eSelectedContest = Contest::ToyCleanUp; printf("Vybrano: ToyCleanUp\n"); }
            
    //         // Potvrzení výběru (přesun do dalšího MENU)
    //         if (rkButtonOn(true)) {
    //             if (eSelectedContest == Contest::Roadside) {
    //                 eCurrentState = MenuState::ROADSIDE_SELECT_TEAM;
    //                 printf("Presun do: ROADSIDE MENU 1 (Vyber tymu)\n");
    //             } else if (eSelectedContest == Contest::ToyCleanUp) {
    //                 eCurrentState = MenuState::TOYCLEANUP_WAIT_START;
    //                 printf("Presun do: TOYCLEANUP MENU 1 (Cekam na START)\n");
    //             } else {
    //                 printf("Nejprve vyber soutez pomoci Button1 nebo Button2!\n");
    //             }
    //         }
    //         break;

    //     // ========================================================
    //     // ROADSIDE MENU 1: Výběr Týmu (Barvy)
    //     // ========================================================

    //     case MenuState::ROADSIDE_SELECT_TEAM:
    //         // Signalizace podle vybraného týmu
    //         rkLedAll(false);
    //         if (eSelectedTeam == TeamColor::Blue) rkLedBlue(true);
    //         else if (eSelectedTeam == TeamColor::Red) rkLedRed(true);

    //         if (rkButton1(true)) { eSelectedTeam = TeamColor::Blue; printf("Tym: MODRA\n"); }
    //         if (rkButton2(true)) { eSelectedTeam = TeamColor::Red; printf("Tym: CERVENA\n"); }
            
    //         // Potvrzení MENU
    //         if (rkButtonOn(true)) {
    //             eCurrentState = MenuState::ROADSIDE_SELECT_LAYOUT;
    //             printf("Presun do: ROADSIDE MENU 2 (Vyber rozlozeni)\n");
    //         }
    //         // Návrat do predchozího MENU
    //         if (rkButtonOff(true)) {
    //             eCurrentState = MenuState::SELECT_CONTEST;
    //             printf("Zpet na: Vyber souteze (Contest)\n");
    //         }
    //         break;

    //     // ========================================================
    //     // ROADSIDE MENU 2: Výběr Rozložení Baterií
    //     // ========================================================

    //     case MenuState::ROADSIDE_SELECT_LAYOUT:
    //         // Signalizace rozložení (0-3) pomocí barev
    //         rkLedAll(false);
    //         if (iSelectedLayout == 0) rkLedRed(true);
    //         else if (iSelectedLayout == 1) rkLedYellow(true);
    //         else if (iSelectedLayout == 2) rkLedGreen(true);
    //         else if (iSelectedLayout == 3) rkLedBlue(true);

    //         // -- Výběr hodnot
    //         if (rkButton1(true)) { 
    //             iSelectedLayout = (iSelectedLayout + 1) % 4; 
    //             printf("Rozlozeni (NEXT): %d\n", iSelectedLayout); 
    //         }
    //         if (rkButton2(true)) { 
    //             iSelectedLayout = (iSelectedLayout - 1 < 0) ? 3 : iSelectedLayout - 1; 
    //             printf("Rozlozeni (BACK): %d\n", iSelectedLayout); 
    //         }

    //         // Potvrzení MENU
    //         if (rkButtonOn(true)) {
    //             eCurrentState = MenuState::ROADSIDE_WAIT_START;
    //             printf("Presun do: ROADSIDE MENU 3 (Cekam na start)\n");
    //         }
    //         // Návrat do predchozího MENU
    //         if (rkButtonOff(true)) {
    //             eCurrentState = MenuState::ROADSIDE_SELECT_TEAM;
    //             printf("Zpet na: Vyber tymu\n");
    //         }
    //         break;

    //     // ========================================================
    //     // ROADSIDE MENU 3: START (ON)
    //     // ========================================================

    //     case MenuState::ROADSIDE_WAIT_START:
    //         // Blikání všech LED na znamení připravenosti ke startu
    //         rkLedAll(millis() % 500 < 250);

    //         // Potvrzení MENU ---> Spuštění Roadside
    //         if (rkButtonOn(true)) {
    //             rkLedAll(false); // Vypnutí LED
    //             RoadsideGame.fInitGame(iSelectedLayout, eSelectedTeam);
    //             bRoadsideGameStarted = true;
    //             eCurrentState = MenuState::GAME_RUNNING;
    //             GameTimer.fStart(); // Spuštění vlákna pro odpočet času
    //             printf("=== ROADSIDE ZAPAS ODSTARTOVAN! ===\n");
    //         }
    //         // Návrat do predchozího MENU
    //         if (rkButtonOff(true)) {
    //             eCurrentState = MenuState::ROADSIDE_SELECT_LAYOUT;
    //             printf("Zpet na: Vyber rozlozeni\n");
    //         }
    //         break;

    //     // ========================================================
    //     // TOYCLEANUP MENU 1: START (ON)
    //     // ========================================================

    //     case MenuState::TOYCLEANUP_WAIT_START:
    //         // Blikání zelené - připravenost k TOYCLEANUP
    //         rkLedAll(false);
    //         rkLedGreen(millis() % 500 < 250);

    //         // Potvrzení MENU ---> Spuštění ToyClenUp
    //         if (rkButtonOn(true)) {
    //             rkLedAll(false); 
    //             // Zde můžeš v budoucnu zavolat něco jako ToyCleanUpGame.fInitGame()
    //             bRoadsideGameStarted = true; // Pokud to používáš jako globální flag jízdy
    //             eCurrentState = MenuState::GAME_RUNNING;
    //             GameTimer.fStart(); // Spuštění vlákna pro odpočet času
    //             printf("=== TOYCLEANUP ZAPAS ODSTARTOVAN! ===\n");
    //         }
    //         // Návrat do predchozího MENU
    //         if (rkButtonOff(true)) {
    //             eCurrentState = MenuState::SELECT_CONTEST;
    //             printf("Zpet na: Vyber souteze (Contest)\n");
    //         }
    //         break;

    //     // ========================================================
    //     // FÁZE 2: HLAVNÍ JÍZDNÍ SMYČKA (Po odstartování)
    //     // ========================================================

    //     case MenuState::GAME_RUNNING:
            
    //         if (eSelectedContest == Contest::Roadside) {
    //             // Zjistíme, jestli nám nedochází čas (limit 300s, tolerance např. 45s pro návrat)
    //             if (GameTimer.bIsTimeRunningOut(300, 45)) {
    //                 printf("[CAS!] Zbyva malo casu (ubehlo %d s)! Ukoncuji sber a vracim se na start...\n", GameTimer.iGetElapsedSeconds());
    //                 // Následoval by kód pro odjezd domů
    //             }

    //             // Tady už běží samotná odometrie a logika soutěže ROADSIDE
                
    //             // Zkusi sebrat baterii s vyhýbáním překážkám
    //             RoadsideGame.fTakeClosestBattery(rCurrentRobotX);
                
    //         } 
    //         else if (eSelectedContest == Contest::ToyCleanUp) {
    //             // Zjistíme, jestli nám nedochází čas (limit 150s, tolerance např. 30s)
    //             if (GameTimer.bIsTimeRunningOut(150, 30)) {
    //                 printf("[CAS!] Zbyva malo casu (ubehlo %d s)! Ukoncuji sber a vracim se...\n", GameTimer.iGetElapsedSeconds());
    //                 // Následoval by kód pro odjezd domů
    //             }
    //             // Tady běží kód pro TOYCLEANUP
    //             // printf("Hraju ToyCleanUp...\n");
    //         }
    //         break;
    // }
// ======= TESTOVACÍ ČÁSTI ==========


    if (rkButtonIsPressed(BTN_LEFT))      { rkSmartServoMove(6, 60); }
    if (rkButtonIsPressed(BTN_RIGHT))     { fMoveGrabber(0, iClose_Grabber_RA); }
    if (rkButtonIsPressed(BTN_UP))        {  }
    if (rkButtonIsPressed(BTN_DOWN))      {  }
    if (rkButtonIsPressed(BTN_ON))        {  }
    // if (rkButtonIsPressed(BTN_OFF))     { 
        
    //     // SCÉNÁŘ PRO MANIPULÁTOR 1 (ID 0)
        
    //     fMoveGrabber(0, iOpen_Grabber_RA);      // 1. Otevři chapadlo
    //     delay(1500);
        
    //     fMoveManipulator(0, 180, 65, 0);        // 2. Dojeď pro předmět před sebe                       | Cíl: X=180, Y=50, Základna = 0° (vpřed)
    //     delay(10000);
        
    //     fMoveGrabber(0, iClose_Grabber_RA);     // 3. Zavři chapadlo (chytni předmět)
    //     delay(1500);    
        
    //     fMoveManipulator(0, 180, 150, 0);     // 4. Zvedni ho do výšky a otoč se s ním doprava        | Cíl: X=180, Y=50, Základna = -90° (vpravo)
    //     delay(10000);
        
    //     fMoveGrabber(0, iOpen_Grabber_RA);      // 5. Otevři chapadlo (pusť předmět)
    //     delay(1500);

    //     printf("\n");
        
    // }

    // Ruční nastavování pozice Manipulátoru
    // if (rkButtonIsPressed(BTN_UP))      { rkSmartServoMove(1, rkSmartServosPosicion(1) + 5); }
    // if (rkButtonIsPressed(BTN_DOWN))    { rkSmartServoMove(1, rkSmartServosPosicion(1) - 5 ); } 
    // if (rkButtonIsPressed(BTN_LEFT))    { rkSmartServoMove(2, rkSmartServosPosicion(2) + 5); }
    // if (rkButtonIsPressed(BTN_RIGHT))   { rkSmartServoMove(2, rkSmartServosPosicion(2) - 5);}

    // Ne-blokující výpis hodnot senzorů každých 500 ms (aby se nesekal zbytek robota)
    if (millis() - lastSensorPrint > 500) {
        lastSensorPrint = millis();
        
        int dist = -1;
        dist = rk_laser_measure("laser"); // Měření vzdálenosti z laseru
        
        // Korekce nepřesnosti laseru (ukazoval 200 mm při reálných 155 mm)
        if (dist > 0) {
            dist -= 45;
            if (dist < 0) dist = 0; // Ochrana proti záporným hodnotám při těsném přiblížení
        }
        
        float r, g, b;
        bool color_ok = rkColorSensorGetRGB("color", &r, &g, &b); // Přečtení barev
        
        if (color_ok) {
            printf("[Senzory] Laser: %4d mm | Barva RGB: (%3.0f, %3.0f, %3.0f)\n", dist, r, g, b);
        } else {
            printf("[Senzory] Laser: %4d mm | Barva RGB: Chyba cteni\n", dist);
        }
    }

    delay(10);
}
