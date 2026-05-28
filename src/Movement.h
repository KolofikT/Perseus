#pragma once

#include <Arduino.h>
#include <functional>
#include <cmath>
#include "robotka.h"

struct MoveResult {
    bool success;
    float traveled_mm; // Skutečně ujetá vzdálenost se znaménkem (+ vpřed, - vzad)
};

/**
 * \brief Plynulý pohyb s vyhýbáním se (robot nenarazí).
 * 
 * Používá sinusovou (S-curve) rampu pro ultra-plynulý rozjezd a zpomalení,
 * PI-regulátor pro udržení směru a statický bias pro kompenzaci 
 * asymetrického tření (pravá strana drží víc).
 * 
 * \param mm Vzdálenost v milimetrech (kladná pro jízdu vpřed, záporná pro vzad).
 * \param speed Rychlost v % (0-100).
 * \param is_obstacle Funkce/lambda, která vrátí true, pokud je detekována překážka.
 * \param wait_timeout_ms Jak dlouho (v ms) robot maximálně počká, než vyhodí chybu (výchozí 5s).
 * \return MoveResult struktura s úspěšností (success) a skutečně ujetou vzdáleností (traveled_mm).
 */
inline MoveResult move_acc_avoid(float mm, float speed, std::function<bool()> is_obstacle, uint32_t wait_timeout_ms = 5000) {
    if (mm == 0 || speed == 0) { return {true, 0.0f}; }
    
    // Podpora pro jízdu pozpátku
    bool reverse = (mm < 0);
    float target_mm = std::abs(mm); // Cílová vzdálenost bude vždy kladná (pro výpočet ušlé dráhy)
    
    // Normalizace rychlosti (aby znaménko rychlosti odpovídalo směru)
    float abs_target_speed = std::abs(speed);
    float speed_sign = reverse ? -1.0f : 1.0f;
    
    // Vynulování odometrie
    rkMotorsSetPositionLeft(0);
    rkMotorsSetPositionRight(0);
    delay(50); // Krátká pauza, aby se koprocesor stihl zresetovat
    
    // ============================================================
    // PARAMETRY RAMPY (S-křivka)
    // ============================================================
    float min_speed = 12.0f;         // Nižší startovací rychlost pro plynulost (bylo 25)
    if (abs_target_speed < min_speed) { abs_target_speed = min_speed; }
    
    // Časy pro S-křivku (v ms)
    float accel_duration_ms = 600.0f;  // Doba rozjezdu (bylo ~300ms, teď 600ms pro hladkost)
    
    // Vzdálenost pro brzdění — dynamicky, ale max polovina dráhy
    float decel_distance_mm = std::min(target_mm * 0.4f, 1.2f * abs_target_speed);
    
    // Rychlý stop před překážkou
    float avoid_decel_step = abs_target_speed / 5.0f;
    
    // ============================================================
    // PARAMETRY PI-REGULÁTORU pro udržení směru
    // ============================================================
    float kp = 0.8f;          // Proporcionální (sníženo z 1.5 — méně agresivní)
    float ki = 0.02f;         // Integrální složka (eliminuje trvalou úchylku)
    float max_corr = 10.0f;   // Max korekce (sníženo z 15)
    float integral = 0.0f;    // Akumulátor integrální složky
    float max_integral = 50.0f; // Anti-windup limit
    
    // ============================================================
    // KOMPENZACE ASYMETRICKÉHO TŘENÍ (pravá strana drhne víc)
    // ============================================================
    // Kladná hodnota = pravý motor dostane víc výkonu
    // Experimentálně doladit: začni s 1.5, pokud robot stále uhýbá doprava, zvyš na 2-3
    float right_bias = 1.5f;
    
    // ============================================================
    // ČASOVAČE A STAVOVÉ PROMĚNNÉ
    // ============================================================
    unsigned long start_time = millis();
    unsigned long accel_start_time = millis(); // Čas začátku rozjezdu
    unsigned long avoid_wait_start = 0;
    bool waiting_for_obstacle = false;
    bool accel_phase = true;  // Jsme ve fázi rozjezdu?
    
    // Výpočet hrubého timeoutu pro celou cestu
    float speed_mm_per_sec = abs_target_speed * 5.0f; // odhad 100% speed ~ 500 mm/s
    float expected_time_ms = (target_mm / speed_mm_per_sec) * 1000.0f;
    uint32_t general_timeout_ms = (uint32_t)(expected_time_ms * 2.5f + 4000.0f);
    
    // Pomocná funkce pro bezpečné ořezání hodnoty a převod do int8_t
    auto clamp_speed = [](float s) -> int8_t {
        if (s > 100.0f) return 100;
        if (s < -100.0f) return -100;
        return static_cast<int8_t>(s);
    };

    float avg_pos = 0.0f;
    float current_base_speed = 0.0f; // Startujeme od nuly!

    while (true) {
        float pos_l = rkMotorsGetPositionLeft(true); // 'true' donutí vyčíst reálnou hodnotu od koprocesoru!
        float pos_r = rkMotorsGetPositionRight(true);
        float abs_l = std::abs(pos_l);
        float abs_r = std::abs(pos_r);
        avg_pos = (abs_l + abs_r) / 2.0f;
        
        if (avg_pos >= target_mm) {
            avg_pos = target_mm; // Oříznutí na cíl pro přesnost návratové hodnoty
            break; // Dojeli jsme do cíle
        }
        
        // Časový limit pro celou funkci (pokud se někde nezasekne natrvalo)
        if ((millis() - start_time > general_timeout_ms) && !waiting_for_obstacle) {
            rkMotorsSetSpeed(0, 0);
            return {false, reverse ? -avg_pos : avg_pos}; 
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
                current_base_speed = abs_curr;
                // Reset integrátoru při zastavení
                integral = 0.0f;
            } else {
                // Fáze: Zastaveno, čekáme na volnou cestu
                current_base_speed = 0;
                if (millis() - avoid_wait_start > wait_timeout_ms) {
                    rkMotorsSetSpeed(0, 0);
                    return {false, reverse ? -avg_pos : avg_pos}; // Překážka nezmizela v limitu
                }
            }
        } else {
            if (waiting_for_obstacle) {
                // Překážka právě zmizela, pokračujeme v jízdě
                waiting_for_obstacle = false;
                start_time += (millis() - avoid_wait_start); // Posuneme celkový timeout
                accel_start_time = millis(); // Restart S-křivky od začátku
                accel_phase = true;
                current_base_speed = 0.0f;
                integral = 0.0f;
            }
            
            float dist_remaining = target_mm - avg_pos;
            
            if (dist_remaining <= decel_distance_mm) {
                // ========================================
                // FÁZE ZPOMALENÍ — S-křivka (sinusová)
                // ========================================
                accel_phase = false;
                
                // Normalizovaný progress brzdění (0.0 = začátek brzdění, 1.0 = cíl)
                float decel_progress = 1.0f - (dist_remaining / decel_distance_mm);
                decel_progress = std::max(0.0f, std::min(decel_progress, 1.0f));
                
                // S-křivka: sin() dává plynulý přechod
                float decel_factor = 0.5f * (1.0f + cosf(decel_progress * M_PI)); // 1.0 → 0.0 plynule
                
                current_base_speed = min_speed + (abs_target_speed - min_speed) * decel_factor;
                if (current_base_speed < min_speed) current_base_speed = min_speed;
                
            } else if (accel_phase) {
                // ========================================
                // FÁZE ZRYCHLENÍ — S-křivka (sinusová)
                // ========================================
                float elapsed_ms = (float)(millis() - accel_start_time);
                
                if (elapsed_ms >= accel_duration_ms) {
                    // Rozjezd dokončen
                    current_base_speed = abs_target_speed;
                    accel_phase = false;
                } else {
                    // S-křivka: sin() místo lineárního kroku
                    // progress 0→1, sin(0→π/2) = 0→1, ale S-tvar dává:
                    float accel_progress = elapsed_ms / accel_duration_ms;
                    float accel_factor = 0.5f * (1.0f - cosf(accel_progress * M_PI)); // 0.0 → 1.0 plynule (S-tvar)
                    
                    current_base_speed = min_speed + (abs_target_speed - min_speed) * accel_factor;
                }
            } else {
                // ========================================
                // FÁZE KONSTANTNÍ RYCHLOSTI
                // ========================================
                current_base_speed = abs_target_speed;
            }
        }
        
        // ============================================================
        // APLIKACE PI-REGULÁTORU + BIAS pro udržení směru
        // ============================================================
        if (current_base_speed > 0.5f) {
            float diff = abs_l - abs_r; // Kladné -> levé kolo je napřed
            
            // PI regulátor
            integral += diff;
            // Anti-windup: omezení integrální složky
            if (integral > max_integral) integral = max_integral;
            if (integral < -max_integral) integral = -max_integral;
            
            float correction = diff * kp + integral * ki;
            correction = std::max(-max_corr, std::min(correction, max_corr));
            
            float applied_speed = current_base_speed * speed_sign;
            
            float speed_l = applied_speed;
            float speed_r = applied_speed;
            
            // Aplikace statického biasu pro pravou stranu (kompenzace tření)
            // right_bias je přidáno k pravému motoru vždy
            speed_r += right_bias * speed_sign;
            
            // Pokud je levé kolo napřed, zpomalíme levé a zrychlíme pravé (a naopak)
            speed_l -= correction * speed_sign; 
            speed_r += correction * speed_sign;
            
            rkMotorsSetSpeed(clamp_speed(speed_l), clamp_speed(speed_r));
        } else {
            rkMotorsSetSpeed(0, 0);
        }
        delay(15); // Prodlouženo z 10 na 15ms pro stabilnější regulaci
    }
    
    rkMotorsSetSpeed(0, 0);
    return {true, reverse ? -avg_pos : avg_pos};
}
