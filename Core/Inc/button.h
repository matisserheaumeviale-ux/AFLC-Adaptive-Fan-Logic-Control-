#pragma once

#include <stdbool.h>

void Button_Init(void);
void Button_Task(void);
bool Button_IsConfirmPressed(void);
void Button_ClearConfirm(void);
void Button_NotifyConfirmCommand(void);
