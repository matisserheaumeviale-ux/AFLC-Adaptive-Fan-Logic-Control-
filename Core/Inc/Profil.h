#pragma once

#include <stdbool.h>
#include <stdint.h>

/*
 * Module Profil
 * -------------
 * Ce module prepare les parametres de base utilises par la regulation.
 *
 * Chaque ventilateur a ici un "profil" avec :
 * - une zone de temperature
 * - un RPM minimum
 * - un RPM nominal
 * - un RPM maximum
 *
 * Le resultat est stocke dans une structure simple relue ensuite
 * par AFLCalcul.c et l'interface LCD.
 */

// Le systeme gere 4 ventilateurs.
#define PROFIL_FAN_COUNT 4U

// Identifiant logique du profil applique a un fan.
typedef enum {
    PROFIL_PROFILE_SILENT = 0,
    PROFIL_PROFILE_NORMAL,
    PROFIL_PROFILE_PERFORMANCE,
    PROFIL_PROFILE_BOOST
} Profil_ProfileId_t;

// Parametres calcules pour un ventilateur.
// gain_num / gain_den sert a faire une interpolation lineaire.
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

// Bloc resultat global.
// ready = calcul termine
// valid = contenu exploitable
typedef struct {
    bool ready;
    bool valid;
    Profil_Params_t fans[PROFIL_FAN_COUNT];
} Profil_Result_t;

// Prepare l'etat interne.
// A appeler une fois au boot.
void Profil_Init(void);

// Construit tous les profils par defaut.
// Cette fonction remplit la structure globale resultat.
void Profil_CalculateAll(void);

// Retourne le resultat courant.
// Le pointeur reste gere par le module.
const Profil_Result_t *Profil_GetResult(void);

// Retourne un nom court pour affichage UART/LCD.
const char *Profil_GetName(Profil_ProfileId_t id);
