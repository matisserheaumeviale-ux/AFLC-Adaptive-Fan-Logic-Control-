#pragma once

// Affiche le menu UART au boot.
void UART_Cmd_Init(void);

// Lit les caracteres recus et execute les commandes.
void UART_Cmd_Task(void);
