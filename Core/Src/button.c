#include "button.h"

// Latch logiciel.
// Ici on simule le bouton avec une commande UART.
static bool g_confirm_latched;

void Button_Init(void)
{
    g_confirm_latched = false;
}

void Button_Task(void)
{
    // Reserve pour un vrai polling GPIO plus tard.
}

bool Button_IsConfirmPressed(void)
{
    return g_confirm_latched;
}

void Button_ClearConfirm(void)
{
    g_confirm_latched = false;
}

void Button_NotifyConfirmCommand(void)
{
    // On memorise l'evenement jusqu'a ce que l'app le consomme.
    g_confirm_latched = true;
}
