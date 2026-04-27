#include "AFLCalcul.h"

void AFLCalcul_Compute(const Profil_Result_t *profiles,
                       const TemperatureSnapshot_t *temps,
                       const TachometerSnapshot_t *tach,
                       AFLCalcul_Output_t *out)
{
    uint8_t index;

    // Garde-fou de base.
    // Si un pointeur est NULL, on ne touche a rien.
    if ((profiles == NULL) || (temps == NULL) || (tach == NULL) || (out == NULL)) {
        return;
    }

    for (index = 0U; index < PROFIL_FAN_COUNT; index++) {
        const Profil_Params_t *profile = &profiles->fans[index];
        uint8_t temperature = temps->values_c[index];
        uint32_t target_rpm;

        // Zone 1 : temperature sous le minimum.
        // On garde le RPM minimal du profil.
        if (temperature <= profile->temp_min_c) {
            target_rpm = profile->min_rpm;
        // Zone 2 : temperature au-dessus du maximum.
        // On force le RPM maximal.
        } else if (temperature >= profile->temp_max_c) {
            target_rpm = profile->max_rpm;
        // Zone 3 : interpolation lineaire entre min et max.
        } else {
            uint32_t delta_temp = (uint32_t)(temperature - profile->temp_min_c);
            target_rpm = (uint32_t)profile->min_rpm
                       + ((uint32_t)profile->gain_num * delta_temp) / profile->gain_den;
        }

        // Si le fan tourne deja mais accuse un retard,
        // on evite de demander trop bas.
        // Ca aide le fan a rester dans une zone stable.
        if (tach->fans[index].signal_present && (tach->fans[index].rpm_filtered + 200U < target_rpm)) {
            if (target_rpm < profile->nominal_rpm) {
                target_rpm = profile->nominal_rpm;
            }
        }

        // Clamp final.
        if (target_rpm > profile->max_rpm) {
            target_rpm = profile->max_rpm;
        }

        out->target_rpm[index] = (uint16_t)target_rpm;
    }
}
