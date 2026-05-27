#include <Arduino.h>
#include <Wire.h>
#include "robotka.h"
#include <Adafruit_VL53L0X.h>
#include <Adafruit_TCS34725.h>

// Definice pinů pro XSHUT laserů (uprav čísla pinů podle fyzického zapojení)
#define XSHUT_PIN_1 27
#define XSHUT_PIN_2 25
#define XSHUT_PIN_3 32

// Inicializace instancí senzoru
Adafruit_VL53L0X laser1;
Adafruit_VL53L0X laser2;
Adafruit_VL53L0X laser3;
Adafruit_TCS34725 rgbSensor;

uint32_t lastPrint = 0;

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    rkConfig cfg;
    cfg.pocet_chytrych_serv = 7;
    rkSetup(cfg);

    // 1. Nastavení I2C sběrnice pro Barevný senzor
    Wire.begin(14, 26, 400000);  // SDA = 14, SCL = 26
    Wire.setTimeOut(1);
    
    // 2. Nastavení I2C sběrnice pro Laser
    Wire1.begin(21, 22, 400000); // SDA = 21, SCL = 22
    Wire1.setTimeOut(1);

    Serial.println("Inicializace senzoru...");
    
    // 3. Vypnutí všech laserů přes XSHUT piny (nutné před inicializací)
    pinMode(XSHUT_PIN_1, OUTPUT);
    pinMode(XSHUT_PIN_2, OUTPUT);
    pinMode(XSHUT_PIN_3, OUTPUT);
    
    digitalWrite(XSHUT_PIN_1, LOW);
    digitalWrite(XSHUT_PIN_2, LOW);
    digitalWrite(XSHUT_PIN_3, LOW);
    
    delay(10);

    // 4. Postupná inicializace laserů s unikátními I2C adresami (např. 0x30, 0x31, 0x32)
    rk_laser_init("laser1", Wire1, laser1, XSHUT_PIN_1, 0x30);
    rk_laser_init("laser2", Wire1, laser2, XSHUT_PIN_2, 0x31);
    rk_laser_init("laser3", Wire1, laser3, XSHUT_PIN_3, 0x32);

    // Inicializace barevného senzoru na sběrnici Wire
    rkColorSensorInit("color", Wire, rgbSensor);
    
    Serial.println("Hotovo, jdeme merit!");
}

void loop() {
    if (millis() - lastPrint > 500) {
        lastPrint = millis();
        
        // Měření vzdálenosti z laserů
        int dist1 = rk_laser_measure("laser1");
        int dist2 = rk_laser_measure("laser2");
        int dist3 = rk_laser_measure("laser3");
        
        // Měření barvy z RGB senzoru
        float r, g, b;
        bool color_ok = rkColorSensorGetRGB("color", &r, &g, &b);
        
        // Hezký výpis do terminálu
        if (color_ok) {
            Serial.printf("IR1: %4d mm | IR2: %4d mm | IR3: %4d mm | Barva RGB: (%3.0f, %3.0f, %3.0f)\n", dist1, dist2, dist3, r, g, b);
        } else {
            Serial.printf("IR1: %4d mm | IR2: %4d mm | IR3: %4d mm | Barva RGB: Chyba cteni\n", dist1, dist2, dist3);
        }
    }
    delay(10);
}
