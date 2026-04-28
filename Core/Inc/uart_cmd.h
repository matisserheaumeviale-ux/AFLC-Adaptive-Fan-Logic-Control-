#pragma once

/*
 * Module uart_cmd
 * ---------------
 * Petit interpreteur de commandes serie.
 *
 * Son but est de tester le projet sans interface complexe :
 * on tape un caractere sur l'UART, puis l'action associee est executee.
 */

// Affiche le menu UART au boot.
void UART_Cmd_Init(void);

// Lit les caracteres recus et execute les commandes.
void UART_Cmd_Task(void);
