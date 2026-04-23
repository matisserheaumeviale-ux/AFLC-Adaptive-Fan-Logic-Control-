#include "Profil.h"

typedef struct {
    Profil_ProfileId_t profile_id;
    uint16_t min_rpm;
    uint16_t nominal_rpm;
    uint16_t max_rpm;
    uint8_t temp_min_c;
    uint8_t temp_max_c;
} Profil_Default_t;

static const Profil_Default_t g_profile_defaults[PROFIL_FAN_COUNT] = {
    {PROFIL_PROFILE_SILENT,      700U, 1200U, 1800U, 24U, 48U},
    {PROFIL_PROFILE_BALANCED,    900U, 1500U, 2200U, 26U, 55U},
    {PROFIL_PROFILE_PERFORMANCE, 1100U, 1900U, 2600U, 28U, 60U},
    {PROFIL_PROFILE_BOOST,       1300U, 2200U, 3000U, 30U, 65U}
};

static Profil_Result_t g_result;

void Profil_Init(void)
{
    g_result.ready = false;
    g_result.valid = false;
}

void Profil_CalculateAll(void)
{
    uint8_t index;

    for (index = 0U; index < PROFIL_FAN_COUNT; index++) {
        const Profil_Default_t *source = &g_profile_defaults[index];
        Profil_Params_t *dest = &g_result.fans[index];
        uint16_t temp_span = (uint16_t)(source->temp_max_c - source->temp_min_c);

        dest->profile_id = source->profile_id;
        dest->min_rpm = source->min_rpm;
        dest->nominal_rpm = source->nominal_rpm;
        dest->max_rpm = source->max_rpm;
        dest->temp_min_c = source->temp_min_c;
        dest->temp_max_c = source->temp_max_c;
        dest->gain_num = (uint16_t)(source->max_rpm - source->min_rpm);
        dest->gain_den = (temp_span == 0U) ? 1U : temp_span;
    }

    g_result.ready = true;
    g_result.valid = true;
}

const Profil_Result_t *Profil_GetResult(void)
{
    return &g_result;
}

const char *Profil_GetName(Profil_ProfileId_t id)
{
    switch (id) {
        case PROFIL_PROFILE_SILENT:
            return "SILENT";
        case PROFIL_PROFILE_BALANCED:
            return "BAL";
        case PROFIL_PROFILE_PERFORMANCE:
            return "PERF";
        case PROFIL_PROFILE_BOOST:
            return "BOOST";
        default:
            return "UNK";
    }
}
