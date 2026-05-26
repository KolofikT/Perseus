#include "robotka.h"
#include <Arduino.h>
#include <string>

#ifdef USE_VL53L0X
#include <Adafruit_VL53L0X.h>
#endif
#include <Adafruit_TCS34725.h>

// Globální instance pro senzory a časovač výpisů
Adafruit_VL53L0X laser1;
Adafruit_VL53L0X laser2;
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

// --- Proměnné pro MENU ---
enum class MenuState {
    SELECT_CONTEST,
    ROADSIDE_SELECT_TEAM,
    ROADSIDE_SELECT_LAYOUT,
    ROADSIDE_WAIT_START,
    TOYCLEANUP_WAIT_START,
    GAME_RUNNING
};

enum class Contest { None, Roadside, ToyCleanUp };

MenuState eCurrentState = MenuState::SELECT_CONTEST;
Contest eSelectedContest = Contest::None;
TeamColor eSelectedTeam = TeamColor::Blue; // Výchozí tým
int iSelectedLayout = 0; 
bool bRoadsideGameStarted = false;

float rCurrentRobotX = 1400.0f; 
float rCurrentRobotY = 200.0f;  

void setup() {
    // 1. NEJDŘÍVE nastartovat Serial a počkat, aby se stihl připojit monitor
    Serial.begin(115200);
    delay(1000); 

    printf("\n=== START SETUP ===\n");

    printf("1. Inicializace knihovny Robotka (Serva)...\n");
    // Inicializace knihovny Robotka
    rkConfig cfg;
    cfg.pocet_chytrych_serv = 0;
    // Pokud zrovna nemáš zapojená a napájená chytrá serva, odkomentuj řádek níže, 
    // jinak se rkSetup sekne při jejich hledání a dál tě nepustí!
    // cfg.pocet_chytrych_serv = 0; 
    rkSetup(cfg);

    printf("Robotka started!\n");

    //rkCheckBattery(); 
    
    printf("2. Vypnuti LED na startu...\n");
    rkLedAll(false); // Vypneme LED na startu, menu se o ně postará

    printf("3. Inicializace I2C...\n");
    // Inicializace I2C na zadaných pinech (SDA 21, SCL 22)
    Wire.begin(21, 22);

    printf("4. Nastaveni XSHUT pinu na LOW (vypnuti laseru)...\n");
    // POZOR ZMĚNA: Laser 1 přesunut z GPIO 14 na GPIO 27!
    // GPIO 14 je sdíleno s datovou linkou pro chytrá serva (iservo) a dochází ke konfliktu.
    pinMode(27, OUTPUT);
    pinMode(26, OUTPUT);
    digitalWrite(27, LOW);
    digitalWrite(26, LOW);
    delay(20);

    #ifdef USE_VL53L0X
    printf("5. Inicializace Laser 1 (adresa 0x30)...\n");
    // Postupně probudíme lasery a přesuneme je na nové adresy (0x30 a 0x31)
    rk_laser_init("laser1", Wire, laser1, 27, 0x30); 
    printf("6. Inicializace Laser 2 (adresa 0x31)...\n");
    rk_laser_init("laser2", Wire, laser2, 26, 0x31);
    #endif
    
    printf("7. Inicializace RGB senzoru...\n");
    // RGB senzor se MUSÍ inicializovat až PO laserech, protože mu zůstává fixní adresa 0x29
    rkColorSensorInit("rgb1", Wire, rgbSensor);

    printf("8. Nastaveni chytrych serv...\n");
    // Nastavení Chytrých serv
    // POZOR: Tvůj log ukazuje chybu "Top limit out of range" a limit 2250°. Ujisti se, že zde máš opravdu 225.
    rkSmartServoInit(0, 45, 225, 500, 3); 
    rkSmartServoInit(1, 140, 235);
    rkSmartServoInit(2, 0, 90);
    rkSmartServoInit(3, 0, 120);
    
    printf("=== SETUP DOKONCEN, PRECHAZIM DO LOOP ===\n");
    //rkWaitForStart(); 
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

    
    if (rkButtonIsPressed(BTN_LEFT) && rkButtonIsPressed(BTN_RIGHT))    { fMoveGrabber(0, iOpen_Grabber_RA); }
    if (rkButtonIsPressed(BTN_UP) && rkButtonIsPressed(BTN_DOWN))       { fMoveGrabber(0, iClose_Grabber_TC); }
    if (rkButtonIsPressed(BTN_ON))      { rkSmartServoMove(3, 0); }
    if (rkButtonIsPressed(BTN_OFF))     { rkSmartServoMove(3, 120); }
    // if (rkButtonIsPressed(BTN_OFF))     { 
        
    //     // SCÉNÁŘ PRO MANIPULÁTOR 1 (ID 0)
        
    //     fMoveGrabber(0, iOpen_Grabber_RA);      // 1. Otevři chapadlo
    //     delay(1500);
        
    //     fMoveManipulator(0, 180, 65, 0);        // 2. Dojeď pro předmět před sebe                       | Cíl: X=180, Y=50, Základna = 0° (vpřed)
    //     delay(10000);
        
    //     fMoveGrabber(0, iClose_Grabber_RA);     // 3. Zavři chapadlo (chytni předmět)
    //     delay(1500);    
        
    //     fMoveManipulator(0, 180, 150, -45);     // 4. Zvedni ho do výšky a otoč se s ním doprava        | Cíl: X=180, Y=50, Základna = -90° (vpravo)
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
        
        int d1 = -1, d2 = -1;
        #ifdef USE_VL53L0X
        d1 = rk_laser_measure("laser1"); // Měření vzdálenosti z prvního laseru
        d2 = rk_laser_measure("laser2"); // Měření vzdálenosti z druhého laseru
        #endif
        
        float r, g, b;
        bool rgb_ok = rkColorSensorGetRGB("rgb1", &r, &g, &b); // Přečtení barev
        
        if (rgb_ok) {
            printf("[Senzory] L1: %d mm | L2: %d mm | RGB: (%.0f, %.0f, %.0f)\n", d1, d2, r, g, b);
        } else {
            printf("[Senzory] L1: %d mm | L2: %d mm | RGB: Chyba cteni\n", d1, d2);
        }
    }

    delay(10);
}
