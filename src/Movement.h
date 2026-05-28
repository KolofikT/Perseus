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
 * Používá kubickou rampu (progress³) pro ultra-plynulý rozjezd — z nuly robot
 * sotva šourá a rychlost narůstá exponenciálně. Doba rozjezdu i brzdění
 * závisí na cílové rychlosti (vyšší speed = delší/plynulejší rampa).
 * PI-regulátor pro udržení směru, statický bias pro kompenzaci tření pravé strany.
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
    // PARAMETRY RAMPY — KUBICKÁ KŘIVKA (progress³)
    // ============================================================
    // Kubická křivka: 0.1³=0.001, 0.2³=0.008, 0.3³=0.027, 0.5³=0.125, 0.8³=0.512
    // → První třetina rozjezdu je téměř nehybná, pak to strmě roste
    
    float min_speed = 6.0f;  // Velmi nízká startovací rychlost — robot sotva šourá (bylo 12, pak 25)
    if (abs_target_speed < min_speed) { abs_target_speed = min_speed; }
    
    // Doba rozjezdu ZÁVISÍ NA RYCHLOSTI — vyšší speed = delší rampa
    // speed 100% → 1500ms, speed 50% → 900ms, speed 30% → 660ms
    float accel_duration_ms = 300.0f + 12.0f * abs_target_speed;
    
    // Brzdná vzdálenost ZÁVISÍ NA RYCHLOSTI — vyšší speed = dřívější brzdění
    // speed 100% → min(50%, 200mm), speed 50% → min(50%, 100mm)
    // Zvýšeno oproti předchozímu (bylo 0.4/1.2) aby robot nepřejížděl cíl
    float decel_distance_mm = std::min(target_mm * 0.5f, 2.0f * abs_target_speed);
    
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
    
    // Výpočet hrubého timeoutu pro celou cestu (s extra rezervou pro pomalý rozjezd)
    float speed_mm_per_sec = abs_target_speed * 5.0f; // odhad 100% speed ~ 500 mm/s
    float expected_time_ms = (target_mm / speed_mm_per_sec) * 1000.0f;
    uint32_t general_timeout_ms = (uint32_t)(expected_time_ms * 3.0f + 5000.0f);
    
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
                accel_start_time = millis(); // Restart kubické křivky od začátku
                accel_phase = true;
                current_base_speed = 0.0f;
                integral = 0.0f;
            }
            
            float dist_remaining = target_mm - avg_pos;
            
            if (dist_remaining <= decel_distance_mm) {
                // ========================================
                // FÁZE ZPOMALENÍ — inverzní kubická křivka
                // ========================================
                // Brzdění je zrcadlové k rozjezdu: nejdřív rychle ubírá,
                // pak u cíle pomalu dojíždí (jako kubická z 1→0)
                accel_phase = false;
                
                // progress: 0.0 = začátek brzdění, 1.0 = cíl
                float decel_progress = 1.0f - (dist_remaining / decel_distance_mm);
                decel_progress = std::max(0.0f, std::min(decel_progress, 1.0f));
                
                // Inverzní kubická: (1 - progress)³
                // Při progress=0: factor=1 (plná rychlost)
                // Při progress=0.5: factor=0.125 (už hodně pomalý)
                // Při progress=1.0: factor=0 (zastaveno)
                float inv = 1.0f - decel_progress;
                float decel_factor = inv * inv * inv; // kubická: plynulé dojíždění k cíli
                
                current_base_speed = min_speed + (abs_target_speed - min_speed) * decel_factor;
                if (current_base_speed < min_speed) current_base_speed = min_speed;
                
            } else if (accel_phase) {
                // ========================================
                // FÁZE ZRYCHLENÍ — KUBICKÁ KŘIVKA (progress³)
                // ========================================
                // Kubická křivka dává EXTRA POMALÝ start z nuly:
                //   t=10%:  0.1³ = 0.001 → téměř nula
                //   t=20%:  0.2³ = 0.008 → pořád skoro nic
                //   t=30%:  0.3³ = 0.027 → sotva se hýbe
                //   t=50%:  0.5³ = 0.125 → teprve začíná být vidět
                //   t=70%:  0.7³ = 0.343 → teď to začíná stoupat
                //   t=90%:  0.9³ = 0.729 → strmý nárůst
                //   t=100%: 1.0³ = 1.000 → plná rychlost
                
                float elapsed_ms = (float)(millis() - accel_start_time);
                
                if (elapsed_ms >= accel_duration_ms) {
                    // Rozjezd dokončen
                    current_base_speed = abs_target_speed;
                    accel_phase = false;
                } else {
                    float accel_progress = elapsed_ms / accel_duration_ms; // 0.0 → 1.0
                    float accel_factor = accel_progress * accel_progress * accel_progress; // KUBICKÁ: progress³
                    
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
