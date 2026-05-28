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
 * Používá mocninovou rampu (progress^2.5) pro plynulý rozjezd.
 * Řízení využívá přímé nastavení výkonu motorů (rkMotorsSetPower) namísto regulované rychlosti.
 * Tím eliminujeme vnitřní PID koprocesoru a odstraňujeme dvojitou regulaci a chvění na startu.
 * PI-regulátor je zcela vyřazen při rychlostech < 25% (rozjezd a dojezd),
 * přičemž enkodéry se při přechodu do rychlé fáze virtuálně vynulují, aby se 
 * zamezilo kmitání a náhlému vybočení. Na konci se kola srovnají plazivým výkonem.
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
    
    // ============================================================
    // KOMPENZACE ASYMETRICKÉHO TŘENÍ (pravá strana drhne víc)
    // ============================================================
    // Pravá převodovka drhne v obou směrech. Bias roste s rychlostí robota.
    float static_bias_fw = 2.0f;   // Vybalancováno: startovací bias vpřed (bylo 2.6)
    float dynamic_bias_fw = 2.0f;  // Vybalancováno: přírůstek biasu při plném výkonu vpřed (bylo 2.4)
    float static_bias_bw = 2.0f;   // Zvýšeno: startovací bias vzad (odstraněn chybný záporný bias)
    float dynamic_bias_bw = 1.5f;  // Zvýšeno: přírůstek biasu vzad (odstraněn chybný záporný bias)
    
    float static_bias = reverse ? static_bias_bw : static_bias_fw;
    float dynamic_bias = reverse ? dynamic_bias_bw : dynamic_bias_fw;

    // ============================================================
    // PARAMETRY PI-REGULÁTORU pro udržení směru
    // ============================================================
    float ki = 0.15f;         // Integrální složka (škálovaná s dt = 15ms)
    float max_corr = 10.0f;   // Max korekce
    float integral = 0.0f;    // Akumulátor integrální složky
    float max_integral = 200.0f; // Anti-windup limit

    // Vynulování odometrie
    rkMotorsSetPositionLeft(0);
    rkMotorsSetPositionRight(0);
    delay(50); // Krátká pauza, aby se koprocesor stihl zresetovat
    
    // ============================================================
    // PARAMETRY RAMPY — MOCNINOVÁ KŘIVKA (progress^2.5)
    // ============================================================
    float min_speed = 10.0f;  // Rozumná startovací rychlost (výkon)
    if (abs_target_speed < min_speed) { abs_target_speed = min_speed; }
    
    // Doba rozjezdu ZÁVISÍ NA RYCHLOSTI — vyšší speed = delší rampa
    float accel_duration_ms = 300.0f + 12.0f * abs_target_speed;
    
    // Brzdná vzdálenost ZÁVISÍ NA RYCHLOSTI
    float decel_distance_mm = std::min(target_mm * 0.5f, 2.0f * abs_target_speed);
    
    // Rychlý stop před překážkou
    float avoid_decel_step = abs_target_speed / 5.0f;
    
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
    uint32_t general_timeout_ms = (uint32_t)(expected_time_ms * 3.0f + 5000.0f);
    
    // Pomocná funkce pro bezpečné ořezání hodnoty a převod do int8_t
    auto clamp_speed = [](float s) -> int8_t {
        if (s > 100.0f) return 100;
        if (s < -100.0f) return -100;
        return static_cast<int8_t>(s);
    };

    float avg_pos = 0.0f;
    float current_base_speed = 0.0f; // Startujeme od nuly!

    // Virtuální posuny (offsety) enkodérů pro zamezení skokového dorovnání po zapnutí PI
    float offset_l = 0.0f;
    float offset_r = 0.0f;
    bool regulator_active = false;

    while (true) {
        float pos_l = rkMotorsGetPositionLeft(true); // 'true' donutí vyčíst reálnou hodnotu od koprocesoru!
        float pos_r = rkMotorsGetPositionRight(true);
        float raw_abs_l = std::abs(pos_l);
        float raw_abs_r = std::abs(pos_r);
        
        // Celková průměrná dráha od startu jízdy
        avg_pos = (raw_abs_l + raw_abs_r) / 2.0f;
        
        if (avg_pos >= target_mm) {
            avg_pos = target_mm; // Oříznutí na cíl pro přesnost návratové hodnoty
            break; // Dojeli jsme do cíle
        }
        
        // Časový limit pro celou funkci
        if ((millis() - start_time > general_timeout_ms) && !waiting_for_obstacle) {
            rkMotorsSetPower(0, 0);
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
                integral = 0.0f;
            } else {
                current_base_speed = 0;
                if (millis() - avoid_wait_start > wait_timeout_ms) {
                    rkMotorsSetPower(0, 0);
                    return {false, reverse ? -avg_pos : avg_pos}; 
                }
            }
        } else {
            if (waiting_for_obstacle) {
                waiting_for_obstacle = false;
                start_time += (millis() - avoid_wait_start);
                accel_start_time = millis();
                accel_phase = true;
                current_base_speed = 0.0f;
                integral = 0.0f;
            }
            
            float dist_remaining = target_mm - avg_pos;
            
            if (dist_remaining <= decel_distance_mm) {
                // ========================================
                // FÁZE ZPOMALENÍ — mocninová rampa
                // ========================================
                accel_phase = false;
                
                float decel_progress = 1.0f - (dist_remaining / decel_distance_mm);
                decel_progress = std::max(0.0f, std::min(decel_progress, 1.0f));
                
                float inv = 1.0f - decel_progress;
                float decel_factor = powf(inv, 2.5f);
                
                current_base_speed = min_speed + (abs_target_speed - min_speed) * decel_factor;
                if (current_base_speed < min_speed) current_base_speed = min_speed;
                
            } else if (accel_phase) {
                // ========================================
                // FÁZE ZRYCHLENÍ — MOCNINOVÁ RAMPY (progress^2.5)
                // ========================================
                float elapsed_ms = (float)(millis() - accel_start_time);
                
                if (elapsed_ms >= accel_duration_ms) {
                    current_base_speed = abs_target_speed;
                    accel_phase = false;
                } else {
                    float accel_progress = elapsed_ms / accel_duration_ms;
                    float accel_factor = powf(accel_progress, 2.5f);
                    
                    current_base_speed = min_speed + (abs_target_speed - min_speed) * accel_factor;
                }
            } else {
                // Konstantní rychlost
                current_base_speed = abs_target_speed;
            }
        }
        
        // ============================================================
        // APLIKACE PI-REGULÁTORU + BIAS pro udržení směru
        // ============================================================
        if (current_base_speed > 0.5f) {
            float speed_l = current_base_speed * speed_sign;
            float speed_r = current_base_speed * speed_sign;
            
            // Bias pro kompenzaci tření pravé strany — roste úměrně k rychlosti robota
            float speed_ratio = current_base_speed / abs_target_speed;
            float applied_bias = static_bias + dynamic_bias * speed_ratio;
            speed_r += applied_bias * speed_sign;
            
            // PI regulátor aktivujeme POUZE při rychlosti >= 25%.
            // Během pomalého rozjezdu (pod 25%) PI regulátor vůbec nepracuje, čímž eliminujeme
            // jakékoli počáteční chvění a kmitání na startu.
            if (current_base_speed >= 25.0f) {
                if (!regulator_active) {
                    // OKAMŽIK ZAPNUTÍ REGULÁTORU:
                    // Zmrazíme aktuální polohy jako offsety. Odteď měříme dráhu relativně vůči tomuto bodu.
                    // Tím zamezíme jakémukoli škubnutí nebo vybočení, protože PI začíná s čistým štítem (diff = 0).
                    offset_l = raw_abs_l;
                    offset_r = raw_abs_r;
                    regulator_active = true;
                    integral = 0.0f;
                }
                
                // Relativní dráhy od okamžiku aktivace regulace
                float rel_l = raw_abs_l - offset_l;
                float rel_r = raw_abs_r - offset_r;
                float diff = rel_l - rel_r;
                
                float dt = 0.015f;
                integral += diff * dt;
                
                if (integral > max_integral) integral = max_integral;
                if (integral < -max_integral) integral = -max_integral;
                
                // Adaptivní kp: během zrychlování 1.6f pro extrémně těsné držení stopy, 
                // při konstantní rychlosti klesne na 0.8f pro klidný chod.
                float current_kp = accel_phase ? 1.6f : 0.8f;
                
                float correction = diff * current_kp + integral * ki;
                correction = std::max(-max_corr, std::min(correction, max_corr));
                
                speed_l -= correction * speed_sign; 
                speed_r += correction * speed_sign;
            } else {
                // Při pomalé rychlosti (včetně brzdění) PI regulaci vypínáme pro hladký chod bez kmitání.
                // Offsety nenulujeme, abychom zapsali celkovou ujetou dráhu pro závěrečné dojezdové dorovnání.
                if (!regulator_active) {
                    offset_l = raw_abs_l;
                    offset_r = raw_abs_r;
                }
            }
            
            // POUŽÍVÁME rkMotorsSetPower PRO PŘÍMÉ OVLÁDÁNÍ VÝKONU (bypasujeme vnitřní PID koprocesoru)
            rkMotorsSetPower(clamp_speed(speed_l), clamp_speed(speed_r));
        } else {
            rkMotorsSetPower(0, 0);
        }
        delay(15);
    }
    
    // Hlavní jízda dokončena, zastavíme motory
    rkMotorsSetPower(0, 0);
    delay(30); // Krátká pauza pro zklidnění mechanických kmitů
    
    // ============================================================
    // AKTIVNÍ DOJEZDOVÉ DOROVNÁNÍ KOL (Micro-stepping alignment)
    // ============================================================
    // Plazivou silou (8%) dotáhneme zaostávající motor na celkovou shodu enkodérů (<0.8 ticku)
    // To dokonale srovná jakékoli odchylky vzniklé během pomalého rozjezdu a brzdění.
    unsigned long align_start = millis();
    float align_speed = 8.0f;
    
    while (millis() - align_start < 250) { // Nouzový limit 250ms
        float pos_l = rkMotorsGetPositionLeft(true);
        float pos_r = rkMotorsGetPositionRight(true);
        float abs_l = std::abs(pos_l);
        float abs_r = std::abs(pos_r);
        float diff = abs_l - abs_r;
        
        if (std::abs(diff) <= 0.8f) {
            break; // Dokonale srovnaná kola!
        }
        
        if (diff > 0.8f) {
            // Levé kolo ujelo víc, pravé ho pomalu dohání
            rkMotorsSetPower(0, clamp_speed((align_speed + static_bias) * speed_sign));
        } else if (diff < -0.8f) {
            // Pravé kolo ujelo víc, levé ho pomalu dohání
            rkMotorsSetPower(clamp_speed(align_speed * speed_sign), 0);
        }
        delay(10);
    }
    
    // Úplné finální zastavení a odečet přesné celkové dráhy
    rkMotorsSetPower(0, 0);
    float final_pos = (std::abs(rkMotorsGetPositionLeft(true)) + std::abs(rkMotorsGetPositionRight(true))) / 2.0f;
    return {true, reverse ? -final_pos : final_pos};
}
