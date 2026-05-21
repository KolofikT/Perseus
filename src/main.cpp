#include "robotka.h"
#include <Arduino.h>
#include <string>

// Include ovládacích souborů pro Persea
#include "Manipulator.h"
#include "Roadside.h"


// Nastavení Roadsidu
GameManager RoadsideGame;

// --- Proměnné pro MENU ---
enum class MenuState {
    SELECT_DISCIPLINE,
    ROADSIDE_SELECT_TEAM,
    ROADSIDE_SELECT_LAYOUT,
    ROADSIDE_WAIT_START,
    TOYCLEANUP_WAIT_START,
    GAME_RUNNING
};

enum class Discipline { None, Roadside, ToyCleanUp };

MenuState eCurrentState = MenuState::SELECT_DISCIPLINE;
Discipline eSelectedDiscipline = Discipline::None;
TeamColor eSelectedTeam = TeamColor::Blue; // Výchozí tým
int iSelectedLayout = 0; 
bool bRoadsideGameStarted = false;

void setup() {
    printf("RB3204-RBCX s Robotkou\n");

    // Inicializace knihovny Robotka
    rkConfig cfg;
    rkSetup(cfg);

    printf("Robotka started!\n");
    Serial.begin(115200);

    //rkCheckBattery(); 
    
    rkLedAll(false); // Vypneme LED na startu, menu se o ně postará


    // Nastavení Chytrých serv
    rkSmartServoInit(0, 45, 2250, 500, 3);
    rkSmartServoInit(1, 140, 235);
    rkSmartServoInit(2, 0, 90);
    rkSmartServoInit(3, 0, 120);
    
    //rkWaitForStart(); 
}
void loop() {

    switch (eCurrentState) {
        
        // ========================================================
        // MENU 0: Výběr soutěžní disciplíny
        // ========================================================

        case MenuState::SELECT_DISCIPLINE:
            
            // Signalizace: Modrá pro Roadside, Zelená pro ToyCleanUp, Žlutá pro NIC
            rkLedAll(false);
            if (eSelectedDiscipline == Discipline::Roadside) rkLedBlue(true);
            else if (eSelectedDiscipline == Discipline::ToyCleanUp) rkLedGreen(true);
            else rkLedYellow(true);
            
            // Výběr a uložení volby pomocí BTN_1 a BTN_2
            if (rkButton1(true)) { eSelectedDiscipline = Discipline::Roadside; printf("Vybrano: Roadside\n"); }
            if (rkButton2(true)) { eSelectedDiscipline = Discipline::ToyCleanUp; printf("Vybrano: ToyCleanUp\n"); }
            
            // Potvrzení výběru (přesun do dalšího MENU - dle soutěže)
            if (rkButtonOn(true)) {
                if (eSelectedDiscipline == Discipline::Roadside) {
                    eCurrentState = MenuState::ROADSIDE_SELECT_TEAM;
                    printf("Presun do: ROADSIDE MENU 1 (Vyber tymu)\n");
                } else if (eSelectedDiscipline == Discipline::ToyCleanUp) {
                    eCurrentState = MenuState::TOYCLEANUP_WAIT_START;
                    printf("Presun do: TOYCLEANUP MENU 1 (Cekam na START)\n");
                } else {
                    printf("Nejprve vyber disciplinu pomoci Button1 nebo Button2!\n");
                }
            }
            break;

        // ========================================================
        // ROADSIDE MENU 1: Výběr Týmu (Barvy)
        // ========================================================

        case MenuState::ROADSIDE_SELECT_TEAM:
            // Signalizace podle vybraného týmu
            rkLedAll(false);
            if (eSelectedTeam == TeamColor::Blue) rkLedBlue(true);
            else if (eSelectedTeam == TeamColor::Red) rkLedRed(true);

            if (rkButton1(true)) { eSelectedTeam = TeamColor::Blue; printf("Tym: MODRA\n"); }
            if (rkButton2(true)) { eSelectedTeam = TeamColor::Red; printf("Tym: CERVENA\n"); }
            
            // Potvrzení MENU
            if (rkButtonOn(true)) {
                eCurrentState = MenuState::ROADSIDE_SELECT_LAYOUT;
                printf("Presun do: ROADSIDE MENU 2 (Vyber rozlozeni)\n");
            }
            // Návrat do predchozího MENU
            if (rkButtonOff(true)) {
                eCurrentState = MenuState::SELECT_DISCIPLINE;
                printf("Zpet na: Vyber discipliny\n");
            }
            break;

        // ========================================================
        // ROADSIDE MENU 2: Výběr Rozložení Baterií
        // ========================================================

        case MenuState::ROADSIDE_SELECT_LAYOUT:
            // Signalizace rozložení (0-3) pomocí barev
            rkLedAll(false);
            if (iSelectedLayout == 0) rkLedRed(true);
            else if (iSelectedLayout == 1) rkLedYellow(true);
            else if (iSelectedLayout == 2) rkLedGreen(true);
            else if (iSelectedLayout == 3) rkLedBlue(true);

            // -- Výběr hodnot
            if (rkButton1(true)) { 
                iSelectedLayout = (iSelectedLayout + 1) % 4; 
                printf("Rozlozeni (NEXT): %d\n", iSelectedLayout); 
            }
            if (rkButton2(true)) { 
                iSelectedLayout = (iSelectedLayout - 1 < 0) ? 3 : iSelectedLayout - 1; 
                printf("Rozlozeni (BACK): %d\n", iSelectedLayout); 
            }

            // Potvrzení MENU
            if (rkButtonOn(true)) {
                eCurrentState = MenuState::ROADSIDE_WAIT_START;
                printf("Presun do: ROADSIDE MENU 3 (Cekam na start)\n");
            }
            // Návrat do predchozího MENU
            if (rkButtonOff(true)) {
                eCurrentState = MenuState::ROADSIDE_SELECT_TEAM;
                printf("Zpet na: Vyber tymu\n");
            }
            break;

        // ========================================================
        // ROADSIDE MENU 3: START (ON)
        // ========================================================

        case MenuState::ROADSIDE_WAIT_START:
            // Blikání všech LED na znamení připravenosti ke startu
            rkLedAll(millis() % 500 < 250);

            // Potvrzení MENU ---> Spuštění Roadside
            if (rkButtonOn(true)) {
                rkLedAll(false); // Vypnutí LED
                RoadsideGame.fInitGame(iSelectedLayout, eSelectedTeam);
                bRoadsideGameStarted = true;
                eCurrentState = MenuState::GAME_RUNNING;
                printf("=== ROADSIDE ZAPAS ODSTARTOVAN! ===\n");
            }
            // Návrat do predchozího MENU
            if (rkButtonOff(true)) {
                eCurrentState = MenuState::ROADSIDE_SELECT_LAYOUT;
                printf("Zpet na: Vyber rozlozeni\n");
            }
            break;

        // ========================================================
        // TOYCLEANUP MENU 1: START (ON)
        // ========================================================

        case MenuState::TOYCLEANUP_WAIT_START:
            // Blikání zelené - připravenost k TOYCLEANUP
            rkLedAll(false);
            rkLedGreen(millis() % 500 < 250);

            // Potvrzení MENU ---> Spuštění ToyClenUp
            if (rkButtonOn(true)) {
                rkLedAll(false); 
                // Zde můžeš v budoucnu zavolat něco jako ToyCleanUpGame.fInitGame()
                bRoadsideGameStarted = true; // Pokud to používáš jako globální flag jízdy
                eCurrentState = MenuState::GAME_RUNNING;
                printf("=== TOYCLEANUP ZAPAS ODSTARTOVAN! ===\n");
            }
            // Návrat do predchozího MENU
            if (rkButtonOff(true)) {
                eCurrentState = MenuState::SELECT_DISCIPLINE;
                printf("Zpet na: Vyber discipliny\n");
            }
            break;

        // ========================================================
        // FÁZE 2: HLAVNÍ JÍZDNÍ SMYČKA (Po odstartování)
        // ========================================================

        case MenuState::GAME_RUNNING:
            
            if (eSelectedDiscipline == Discipline::Roadside) {
                // Tady už běží samotná odometrie a logika soutěže ROADSIDE
                float rCurrentRobotX = 1400.0f; 
                int iClosestDockID = RoadsideGame.fFindClosestEmptyDock(rCurrentRobotX);
                
                if (iClosestDockID != -1) {
                    float fX = RoadsideGame.rGetDockPosX(iClosestDockID);
                    printf("Jedu k Docku na X = %.1f\n", fX);
                    while(true); // Zastavení programu pro demonstraci
                }
            } 
            else if (eSelectedDiscipline == Discipline::ToyCleanUp) {
                // Tady běží kód pro TOYCLEANUP
                // printf("Hraju ToyCleanUp...\n");
            }
            break;
    }
// ======= TESTOVACÍ ČÁSTI ==========

    
    if (rkButtonIsPressed(BTN_LEFT) && rkButtonIsPressed(BTN_RIGHT))    { fMoveGrabber(0, iOpen_Grabber_RA); }
    if (rkButtonIsPressed(BTN_UP) && rkButtonIsPressed(BTN_DOWN))       { fMoveGrabber(0, iClose_Grabber_TC); }
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

    // Ruční nastavování pozice Manipulátoru
    // if (rkButtonIsPressed(BTN_UP))      { rkSmartServoMove(1, rkSmartServosPosicion(1) + 5); }
    // if (rkButtonIsPressed(BTN_DOWN))    { rkSmartServoMove(1, rkSmartServosPosicion(1) - 5 ); } 
    // if (rkButtonIsPressed(BTN_LEFT))    { rkSmartServoMove(2, rkSmartServosPosicion(2) + 5); }
    // if (rkButtonIsPressed(BTN_RIGHT))   { rkSmartServoMove(2, rkSmartServosPosicion(2) - 5);}

    delay(10);
}
