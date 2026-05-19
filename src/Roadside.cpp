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
    // Inicializace Docků na základě vylosovaného rozložení
    void fSetupDocks(const std::vector<TeamColor>& vDrawnLayout, TeamColor chosenColor) {
        MyColor = chosenColor;
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

    // 2. Inicializace všech 8 Baterií (2 řady, 4 sloupce)
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

    // 3. NOVÉ: Funkce najde index volné baterie na daných souřadnicích s tolerancí 10 mm
    int fFindBatteryIndexByCoords(float targetX, float targetY) {
        float tolerance = 10.0f; 
        for (int i = 0; i < vBatteries.size(); ++i) {
            // Hledáme jen ty Available a porovnáváme odchylku X a Y
            if (vBatteries[i].Status == BatteryStatus::Available &&
                std::abs(vBatteries[i].Pos.fX - targetX) < tolerance && 
                std::abs(vBatteries[i].Pos.fY - targetY) < tolerance) {
                return i;
            }
        }
        return -1; // -1 znamená, že tam žádná volná baterie není
    }

    // 4. NOVÉ: Funkce, které jen předáš souřadnice a ona baterii rovnou "sebere"
    void fTakeBatteryByCoords(float targetX, float targetY) {
        int idx = fFindBatteryIndexByCoords(targetX, targetY);
        
        if (idx != -1) {
            vBatteries[idx].Status = BatteryStatus::Taken;
            std::cout << "Baterie na [" << targetX << ", " << targetY << "] uspesne sebrana! (Index v poli: " << idx << ")\n";
        } else {
            std::cout << "Chyba: Na [" << targetX << ", " << targetY << "] zadna dostupna baterie neni.\n";
        }
    }
};

// ===================================================

int main() {
    GameManager game;
    
    // Dejme tomu, že jsi vylosoval modrou barvu a rozložení aut je následující:
    std::vector<TeamColor> vMatchLayout = {
        TeamColor::Red, TeamColor::Blue, TeamColor::Red, TeamColor::Red,     // Bližší polovina (1 modré)
        TeamColor::Blue, TeamColor::Blue, TeamColor::Red, TeamColor::Blue    // Vzdálenější polovina (3 modré)
    };

    // 1. Inicializace hřiště
    game.fSetupDocks(vMatchLayout, TeamColor::Blue);
    game.fSetupBatteries();

    // 2. Robot získá své souřadnice z odometrie (např. dojede na druhou bližší baterii)
    // a pošle příkaz k jejímu sebrání v paměti:
    game.fTakeBatteryByCoords(1430.0f, 950.0f);

    // 3. Test pojistky: Zkusíme sebrat tu stejnou baterii podruhé
    // Nyní už by měla vypsat chybu, protože její stav je 'Taken'
    game.fTakeBatteryByCoords(1430.0f, 950.0f);

    return 0;
}