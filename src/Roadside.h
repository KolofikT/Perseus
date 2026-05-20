#pragma once // Zabrání vícenásobnému načtení souboru

#include <iostream>
#include <vector>
#include <cmath> // Přidáno pro matematickou funkci abs

using namespace std; 

// Definice barev týmů
enum class TeamColor {
    Blue,
    Red,
    Unknown
};

// Stavy elektromobilu (Docku)
enum class DockStatus {
    Empty,  // Naše barva, čeká na baterii
    Full,   // Úspěšně vložena baterie
    Enemy   // Barva soupeře
};

// Stavy baterie
enum class BatteryStatus {
    Available, 
    Taken      
};

// Struktura pro X, Y souřadnice v mm
struct Position {
    float fX;
    float fY;
};

// Struktura Docků: X, Y, BARVA, STAV
struct Dock {
    Position Pos;
    TeamColor Color;
    DockStatus Status;
};

// Struktura pro Baterii: X, Y, STAV
struct Battery {
    Position Pos;
    BatteryStatus Status;
};

// ============================================================

class GameManager {
private:
    vector<Dock> vDocks;
    vector<Battery> vBatteries;
    TeamColor MyColor;

public:
    // ============================================================
    // HLAVNÍ SPOUŠTĚČ (Tuto funkci voláš z main.cpp)
    // ============================================================
    void fInitGame(int iButtonClicks, TeamColor ChosenColor) {
        // Všechny kombinace jsou bezpečně schované zde uvnitř
        vector<vector<TeamColor>> vAllLayouts = {
            // Kombinace 0: Naše auto (Blue) je 1. v pořadí zleva
            {TeamColor::Blue, TeamColor::Red, TeamColor::Red, TeamColor::Red,
             TeamColor::Blue, TeamColor::Blue, TeamColor::Blue, TeamColor::Red},

            // Kombinace 1: Naše auto (Blue) je 2. v pořadí zleva
            {TeamColor::Red, TeamColor::Blue, TeamColor::Red, TeamColor::Red,
             TeamColor::Blue, TeamColor::Blue, TeamColor::Red, TeamColor::Blue},

            // Kombinace 2: Naše auto (Blue) je 3. v pořadí zleva
            {TeamColor::Red, TeamColor::Red, TeamColor::Blue, TeamColor::Red,
             TeamColor::Blue, TeamColor::Red, TeamColor::Blue, TeamColor::Blue},

            // Kombinace 3: Naše auto (Blue) je 4. v pořadí zleva
            {TeamColor::Red, TeamColor::Red, TeamColor::Red, TeamColor::Blue,
             TeamColor::Red, TeamColor::Blue, TeamColor::Blue, TeamColor::Blue}
        };

        // Zabezpečení kliknutí a zacyklení
        int iSelectedLayoutIndex = iButtonClicks % 4;
        
        // Použití tlačítka ZPĚT (Navrácení záporné hodnoty [odčítáme: iButtonClicks])
        if (iSelectedLayoutIndex < 0) { iSelectedLayoutIndex += 4; }

        cout << "--- INICIALIZACE HRY ---\n";
        cout << "Pocet kliknuti: " << iButtonClicks << "\n";
        cout << "Vybrany index kombinace: " << iSelectedLayoutIndex << "\n";

        // Vnitřní volání nastavovacích funkcí
        fSetupDocks(vAllLayouts[iSelectedLayoutIndex], ChosenColor);
        fSetupBatteries();
    }

    // =============================================
    // ---- NASTAVENÍ POZIC DOCŮ A BATERIÍ ZDE ----

    // Inicializace Docků na základě vylosovaného rozložení
    void fSetupDocks(const vector<TeamColor>& vDrawnLayout, TeamColor ChosenColor) {
        MyColor = ChosenColor;
        vDocks.clear(); // Pro jistotu vyčistíme předchozí data

        // Fyzické pozice 8 Docků na hřišti v mm
        float rFixedXPositions[8] = {180, 550, 920, 1300, 1700, 2080, 2450, 2820};
        float rFixedYPosition = 100.0f; 

        // Defaultní nastavení všech 8 Docků (Pozice|Stav|Barva)
        for (int i = 0; i < 8; ++i) {
            DockStatus CurrentStatus;
            
            if (vDrawnLayout[i] == MyColor) {
                CurrentStatus = DockStatus::Empty;
            } else {
                CurrentStatus = DockStatus::Enemy;
            }
            
            vDocks.push_back({{rFixedXPositions[i], rFixedYPosition}, vDrawnLayout[i], CurrentStatus}); 
        }
    }

    // Inicializace všech 8 Baterií (2 řady, 4 sloupce)
    void fSetupBatteries() {
        vBatteries.clear();
        
        // X souřadnice pro 4 sloupce (kolem středu 1500 mm)
        float rX_Positions[4] = {1300.0f, 1430.0f, 1570.0f, 1700.0f}; 
        
        // Y souřadnice pro 2 řady (kolem středu 1000 mm)
        float rY_Closer = 950.0f;   // Bližší k autům
        float rY_Farther = 1050.0f; // Vzdálenější od aut
        
        for (int i = 0; i < 4; ++i) {
            // Vložíme bližší baterii
            vBatteries.push_back({{rX_Positions[i], rY_Closer}, BatteryStatus::Available});
            // Vložíme vzdálenější baterii
            vBatteries.push_back({{rX_Positions[i], rY_Farther}, BatteryStatus::Available});
        }
    }

    // ============================================================
    // GETTERY (Získání konkrétní souřadnice X nebo Y Baterie / Docku podle indexu)
    // ============================================================

    float rGetDockPosX(int index) {
        if (index >= 0 && index < vDocks.size()) return vDocks[index].Pos.fX;
        return -1.0f; // Návratová hodnota při chybě
    }

    float rGetDockPosY(int index) {
        if (index >= 0 && index < vDocks.size()) return vDocks[index].Pos.fY;
        return -1.0f;
    }

    float rGetBatteryPosX(int index) {
        if (index >= 0 && index < vBatteries.size()) return vBatteries[index].Pos.fX;
        return -1.0f;
    }

    float rGetBatteryPosY(int index) {
        if (index >= 0 && index < vBatteries.size()) return vBatteries[index].Pos.fY;
        return -1.0f;
    }

    // ============================================================
    // HLEDÁNÍ NEJBLIŽŠÍCH CÍLŮ POUZE V OSE X
    // ============================================================

    // Najde index volné baterie, která je robotovi nejblíže v ose X
    int fFindClosestBattery(float fRobotX) {
        int iBestIndex = -1;
        float rMinDistance = 999999.0f;
        
        for (int i = 0; i < vBatteries.size(); ++i) {
            if (vBatteries[i].Status == BatteryStatus::Available) {
                float rDistX = abs(vBatteries[i].Pos.fX - fRobotX); // Počítá vzdálenost v ose x
                if (rDistX < rMinDistance) {
                    rMinDistance = rDistX;
                    iBestIndex = i;
                }
            }
        }
        return iBestIndex;
    }

    // Najde index volného Docku (naše barva, stav Empty), který je v ose X nejblíže
    int fFindClosestEmptyDock(float fRobotX) {
        int iBestIndex = -1;
        float rMinDistance = 999999.0f;
        
        for (int i = 0; i < vDocks.size(); ++i) {
            if (vDocks[i].Status == DockStatus::Empty) {
                
                float rDistX = abs(vDocks[i].Pos.fX - fRobotX); // Počítáme vzdálenost v ose X
                if (rDistX < rMinDistance) {
                    rMinDistance = rDistX;
                    iBestIndex = i;
                }
            }
        }
        return iBestIndex;
    }

    // ============================================================
    // MANIPULACE
    // ============================================================

    /*
    void fTakeBatteryByCoords(float fTargetX, float fTargetY) {
        float fTolerance = 10.0f; 
        int iIndex = -1;
        
        for (int i = 0; i < vBatteries.size(); ++i) {
            if (vBatteries[i].Status == BatteryStatus::Available &&
                abs(vBatteries[i].Pos.fX - fTargetX) < fTolerance && 
                abs(vBatteries[i].Pos.fY - fTargetY) < fTolerance) {
                iIndex = i;
                break;
            }
        }

        if (iIndex != -1) {
            vBatteries[iIndex].Status = BatteryStatus::Taken;
            cout << "Baterie na [" << fTargetX << ", " << fTargetY << "] uspesne sebrana! (Index v poli: " << iIndex << ")\n";
        } else {
            cout << "Chyba: Na [" << fTargetX << ", " << fTargetY << "] zadna dostupna baterie neni.\n";
        }
    }

    void fFillDock(int iIndex) {
        if (iIndex >= 0 && iIndex < vDocks.size() && vDocks[iIndex].Status == DockStatus::Empty) {
            vDocks[iIndex].Status = DockStatus::Full;
            cout << "Dock [" << iIndex << "] uspesne naplnen!\n";
        }
    } 
    */
};