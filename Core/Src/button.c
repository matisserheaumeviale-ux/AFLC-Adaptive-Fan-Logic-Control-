#include "button.h"

static bool g_confirm_latched;

void Button_Init(void)
{
    g_confirm_latched = false;
}

void Button_Task(void)
{
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
    g_confirm_latched = true;
}
