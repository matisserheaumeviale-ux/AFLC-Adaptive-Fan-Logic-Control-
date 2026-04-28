#pragma once

#include <stdbool.h>

/*
 * Module button
 * -------------
 * Pour l'instant, ce module sert surtout de "latch" logiciel.
 *
 * Dans ce projet, la confirmation utilisateur vient de l'UART.
 * Plus tard, on pourra remplacer cela par une vraie lecture GPIO
 * sans changer le reste de l'application.
 */

// Initialise le mini module bouton.
void Button_Init(void);

// Tache periodique reservee pour du vrai polling plus tard.
void Button_Task(void);

// Retourne si la confirmation est en attente.
bool Button_IsConfirmPressed(void);

// Efface le latch de confirmation.
void Button_ClearConfirm(void);

// Simule un appui confirmation depuis UART.
void Button_NotifyConfirmCommand(void);
