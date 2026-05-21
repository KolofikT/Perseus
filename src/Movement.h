#pragma once

#include <Arduino.h>
#include <functional>
#include <cmath>
#include "robotka.h"

/**
 * \brief Plynulý pohyb s vyhýbáním se (robot nenarazí).
 * 
 * \param mm Vzdálenost v milimetrech (kladná pro jízdu vpřed, záporná pro vzad).
 * \param speed Rychlost v % (0-100).
 * \param is_obstacle Funkce/lambda, která vrátí true, pokud je detekována překážka.
 * \param wait_timeout_ms Jak dlouho (v ms) robot maximálně počká, než vyhodí chybu (výchozí 5s).
 * \return true pokud robot úspěšně dosáhl cíle, false pokud vypršel čas při čekání.
 */
inline bool move_acc_avoid(float mm, float speed, std::function<bool()> is_obstacle, uint32_t wait_timeout_ms = 5000) {
    if (mm == 0 || speed == 0) return true;
    
    // Podpora pro jízdu pozpátku
    bool reverse = (mm < 0);
    if (reverse) {
        mm = -mm; // Cílová vzdálenost bude vždy kladná (pro výpočet ušlé dráhy)
    }
    
    // Normalizace rychlosti (aby znaménko rychlosti odpovídalo směru)
    float abs_target_speed = std::abs(speed);
    float speed_sign = reverse ? -1.0f : 1.0f;
    
    // Vynulování odometrie
    rkMotorsSetPositionLeft(0);
    rkMotorsSetPositionRight(0);
    delay(50); // Krátká pauza, aby se koprocesor stihl zresetovat
    
    float min_speed = 18.0f;
    if (abs_target_speed < min_speed) abs_target_speed = min_speed;
    
    float current_base_speed = min_speed * speed_sign;
    
    // Parametry rampy a P-regulátoru
    float decel_distance_mm = 0.8f * abs_target_speed; // Čím rychleji jede, tím dříve zpomaluje (např. při 50% -> 40 mm)
    float accel_step = abs_target_speed / 20.0f; // Akcelerace na maximum zabere cca 200 ms
    float decel_step = abs_target_speed / 20.0f; 
    float avoid_decel_step = abs_target_speed / 5.0f; // Rychlé zastavení před překážkou (cca 50 ms)
    
    float kp = 0.8f; // Proporcionální konstanta pro P-regulátor na milimetry
    float max_corr = 8.0f; // Maximální zásah regulátoru (%)
    
    unsigned long start_time = millis();
    unsigned long avoid_wait_start = 0;
    bool waiting_for_obstacle = false;
    
    // Výpočet hrubého timeoutu pro celou cestu
    float speed_mm_per_sec = abs_target_speed * 5.0f; // odhad 100% speed ~ 500 mm/s
    float expected_time_ms = (mm / speed_mm_per_sec) * 1000.0f;
    uint32_t general_timeout_ms = (uint32_t)(expected_time_ms * 2.0f + 3000.0f);
    
    // Pomocná funkce pro bezpečné ořezání hodnoty a převod do int8_t
    auto clamp_speed = [](float s) -> int8_t {
        if (s > 100.0f) return 100;
        if (s < -100.0f) return -100;
        return static_cast<int8_t>(s);
    };

    while (true) {
        float pos_l = rkMotorsGetPositionLeft();
        float pos_r = rkMotorsGetPositionRight();
        float abs_l = std::abs(pos_l);
        float abs_r = std::abs(pos_r);
        float avg_pos = (abs_l + abs_r) / 2.0f;
        
        if (avg_pos >= mm) break; // Dojeli jsme do cíle
        
        // Časový limit pro celou funkci (pokud se někde nezasekne natrvalo)
        if ((millis() - start_time > general_timeout_ms) && !waiting_for_obstacle) {
            rkMotorsSetSpeed(0, 0);
            return false; 
        }
        
        bool obstacle = is_obstacle();
        
        if (obstacle) {
            if (!waiting_for_obstacle) {
                // Fáze: Rychle plynule zastavit
                float abs_curr = std::abs(current_base_speed);
                abs_curr -= avoid_decel_step;
                if (abs_curr <= 0) {
                    abs_curr = 0;
                    waiting_for_obstacle = true;
                    avoid_wait_start = millis();
                }
                current_base_speed = abs_curr * speed_sign;
            } else {
                // Fáze: Zastaveno, čekáme na volnou cestu
                current_base_speed = 0;
                if (millis() - avoid_wait_start > wait_timeout_ms) {
                    rkMotorsSetSpeed(0, 0);
                    return false; // Překážka nezmizela v limitu
                }
            }
        } else {
            if (waiting_for_obstacle) {
                // Překážka právě zmizela, pokračujeme v jízdě
                waiting_for_obstacle = false;
                start_time += (millis() - avoid_wait_start); // Posuneme celkový timeout
                current_base_speed = min_speed * speed_sign; 
            }
            
            float dist_remaining = mm - avg_pos;
            if (dist_remaining <= decel_distance_mm) {
                // Fáze: Běžné plynulé zpomalování před cílem
                float abs_curr = std::abs(current_base_speed);
                abs_curr -= decel_step;
                if (abs_curr < min_speed) abs_curr = min_speed;
                current_base_speed = abs_curr * speed_sign;
            } else {
                // Fáze: Zrychlování a konstantní jízda
                float abs_curr = std::abs(current_base_speed);
                if (abs_curr < abs_target_speed) {
                    abs_curr += accel_step;
                    if (abs_curr > abs_target_speed) abs_curr = abs_target_speed;
                }
                current_base_speed = abs_curr * speed_sign;
            }
        }
        
        // Aplikace rychlosti a P-regulátoru pro udržení směru
        if (current_base_speed != 0) {
            float diff = abs_l - abs_r; // Kladné -> levé kolo je napřed
            float correction = std::max(-max_corr, std::min(diff * kp, max_corr));
            
            float speed_l = current_base_speed;
            float speed_r = current_base_speed;
            
            // Pokud je levé kolo napřed, zpomalíme levé a zrychlíme pravé
            speed_l -= correction * speed_sign; 
            speed_r += correction * speed_sign;
            
            rkMotorsSetSpeed(clamp_speed(speed_l), clamp_speed(speed_r));
        } else {
            rkMotorsSetSpeed(0, 0);
        }
        delay(10);
    }
    
    rkMotorsSetSpeed(0, 0);
    return true;
}
