//******************************************************************************
//   Nom du fichier : PORT_XY.h
//   Auteur : Alain Champagne                  
//      Date de création : 21-12-2015 
//        Fichier de déclaration et de définition des fonctions de controle des 
//        éléments d'entrées sorties.
//  
//******************************************************************************
#ifndef PORT_XYH
  #define PORT_XYH

#include <stdbool.h>  

//Predeclaration des fonctions
 void vPort_IN (uint8_t ucPort);
 void vPort_OUT (uint8_t ucPort);
 uint8_t ucReadPort (uint8_t ucPort);
 void vWritePort(uint8_t ucEcrireDataPort, uint8_t ucPort);
 bool bReadPin (uint8_t ucPort, uint8_t ucPin);
 void vWritePin (uint8_t ucPort, uint8_t ucPin, uint8_t ucState);


#endif
