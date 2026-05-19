#include <iostream>
#include <vector>
#include <cmath> // Přidáno pro matematickou funkci std::abs

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
    std::vector<Dock> vDocks;
    std::vector<Battery> vBatteries;
    TeamColor MyColor;

public:

    // =============================================

    // ---- NASTAVENÍ POZIC DOCŮ A BATERIÍ ZDE ----

    // Inicializace Docků na základě vylosovaného rozložení
    void fSetupDocks(const std::vector<TeamColor>& vDrawnLayout, TeamColor ChosenColor) {
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
            
            vDocks.push_back({{rFixedXPositions[i], rFixedYPosition}, vDrawnLayout[i], CurrentStatus}); // Uložení pozice, barvy, statusu Docku
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

    // Najde index volné baterie, která je robotovi nejblíže v ose X
    int fFindClosestBattery(float fRobotX) {
        int iBestIndex = -1;
        float rMinDistance = 999999.0f;
        
        for (int i = 0; i < vBatteries.size(); ++i) {
            if (vBatteries[i].Status == BatteryStatus::Available) {
                float rDistX = std::abs(vBatteries[i].Pos.fX - fRobotX); // Počítá vzdálenost v ose x
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
                
                float rDistX = std::abs(vDocks[i].Pos.fX - fRobotX); // Počítáme vzdálenost v ose X
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
                std::abs(vBatteries[i].Pos.fX - fTargetX) < fTolerance && 
                std::abs(vBatteries[i].Pos.fY - fTargetY) < fTolerance) {
                iIndex = i;
                break;
            }
        }

        if (iIndex != -1) {
            vBatteries[iIndex].Status = BatteryStatus::Taken;
            std::cout << "Baterie na [" << fTargetX << ", " << fTargetY << "] uspesne sebrana! (Index v poli: " << iIndex << ")\n";
        } else {
            std::cout << "Chyba: Na [" << fTargetX << ", " << fTargetY << "] zadna dostupna baterie neni.\n";
        }
    }

    void fFillDock(int iIndex) {
        if (iIndex >= 0 && iIndex < vDocks.size() && vDocks[iIndex].Status == DockStatus::Empty) {
            vDocks[iIndex].Status = DockStatus::Full;
            std::cout << "Dock [" << iIndex << "] uspesne naplnen!\n";
        }
    } 
    */
};


// ===================================================

int main() {
    GameManager Game;
    
    std::vector<TeamColor> vMatchLayout = {
        TeamColor::Red, TeamColor::Blue, TeamColor::Red, TeamColor::Red,     
        TeamColor::Blue, TeamColor::Blue, TeamColor::Red, TeamColor::Blue    
    };

    Game.fSetupDocks(vMatchLayout, TeamColor::Blue);
    Game.fSetupBatteries();

    // -- SIMULACE --
    float fCurrentRobotX = 1400.0f; // Robot stojí někde poblíž středu osy X

    // 1. Hledání nejbližší baterie jen podle X
    int iClosestBatteryID = Game.fFindClosestBattery(fCurrentRobotX);
    if (iClosestBatteryID != -1) {
        float fBaterryPosX = Game.rGetBatteryPosX(iClosestBatteryID);
        float fBaterryPosY = Game.rGetBatteryPosY(iClosestBatteryID);
        std::cout << "Nejblizsi baterie (podle X) ma index: " << iClosestBatteryID << "\n";
        std::cout << "Její souradnice jsou: X=" << fBaterryPosX << ", Y=" << fBaterryPosY << "\n";
    }

    // 2. Hledání nejbližšího Docku jen podle X
    int fClosestDockID = Game.fFindClosestEmptyDock(fCurrentRobotX);
    if (fClosestDockID != -1) {
        float fDockPosX = Game.rGetDockPosX(fClosestDockID);
        float fDockPosY = Game.rGetDockPosY(fClosestDockID);
        std::cout << "\nNejblizsi volny Dock (podle X) ma index: " << fClosestDockID << "\n";
        std::cout << "Jeho souradnice jsou: X=" << fDockPosX << ", Y=" << fDockPosY << "\n";
    }

    return 0;
}