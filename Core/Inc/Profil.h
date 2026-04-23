#pragma once

#include <stdbool.h>
#include <stdint.h>

#define PROFIL_FAN_COUNT 4U

typedef enum {
    PROFIL_PROFILE_SILENT = 0,
    PROFIL_PROFILE_BALANCED,
    PROFIL_PROFILE_PERFORMANCE,
    PROFIL_PROFILE_BOOST
} Profil_ProfileId_t;

typedef struct {
    Profil_ProfileId_t profile_id;
    uint16_t min_rpm;
    uint16_t nominal_rpm;
    uint16_t max_rpm;
    uint8_t temp_min_c;
    uint8_t temp_max_c;
    uint16_t gain_num;
    uint16_t gain_den;
} Profil_Params_t;

typedef struct {
    bool ready;
    bool valid;
    Profil_Params_t fans[PROFIL_FAN_COUNT];
} Profil_Result_t;

void Profil_Init(void);
void Profil_CalculateAll(void);
const Profil_Result_t *Profil_GetResult(void);
const char *Profil_GetName(Profil_ProfileId_t id);
