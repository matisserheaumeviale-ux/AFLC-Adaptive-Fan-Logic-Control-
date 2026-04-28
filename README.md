# AFLC - Adaptive Fan Logic Control

## But du projet

Ce projet pilote 4 ventilateurs sur une carte **STM32F103 Blue Pill**.

Le principe general est simple :

1. le microcontroleur lit la vitesse reelle des ventilateurs avec les signaux **tachymetre**
2. il calcule une vitesse cible en fonction d'un **profil** et d'une **temperature**
3. il convertit cette cible en **PWM**
4. il affiche l'etat du systeme sur un **LCD 20x4** et sur la **liaison UART**

Ce depot contient donc a la fois :

- du code materiel genere par STM32CubeMX
- du code applicatif ecrit pour la logique AFLC

## Idee globale de l'application

Le projet fonctionne comme une petite machine a etats dans `main.c`.

Ordre de demarrage :

1. initialisation HAL + horloge + GPIO + UART + timers
2. demarrage des peripheriques utiles au projet
3. calcul des profils de ventilation
4. affichage des profils sur le LCD
5. attente d'une confirmation utilisateur par UART
6. test de demarrage des 4 ventilateurs
7. si tout va bien : regulation normale
8. si un ventilateur echoue : passage en mode securite

## Fichiers importants

### Fichiers coeur de l'application

- `Core/Src/main.c`
  Role : point d'entree du programme, scheduler cooperatif, machine a etats principale.
- `Core/Src/AFLCalcul.c`
  Role : coeur du calcul. Transforme temperature + profil + tachymetre en RPM cibles.
- `Core/Src/Profil.c`
  Role : construit les profils par defaut pour chaque ventilateur.
- `Core/Src/fan_control.c`
  Role : applique la commande PWM aux ventilateurs et deduit leur etat logique.
- `Core/Src/tachometer.c`
  Role : mesure les periodes tach et convertit cela en RPM stables.
- `Core/Src/temperature_stub.c`
  Role : simule les temperatures pour tester la regulation sans vrai capteur.
- `Core/Src/uart_cmd.c`
  Role : petit shell serie pour debug, test et validation.

### Fichiers interface utilisateur

- `Core/Src/LCD.c`
  Role : pilote bas niveau du LCD HD44780 en mode 4 bits.
- `Core/Src/ui_lcd.c`
  Role : ecrans applicatifs du LCD selon l'etat du systeme.
- `Core/Src/LCD_UI.c`
  Role : helpers de rendu texte plus compacts pour l'affichage ventilateur.
- `Core/Src/led_status.c`
  Role : code des LEDs d'etat.

### Fichiers STM32 / configuration

- `Core/Src/gpio.c`
  Role : configuration des broches.
- `Core/Src/tim.c`
  Role : configuration des timers.
  `TIM2` sert au tachymetre, `TIM3` au PWM des ventilateurs.
- `Core/Src/usart.c`
  Role : configuration de l'UART.
- `Core/Src/stm32f1xx_it.c`
  Role : interruptions STM32.

## Architecture logique

### 1. `main.c`

`main.c` orchestre tout.

Il contient :

- les periodes des taches
- le contexte global de l'application
- la boucle infinie
- la machine a etats principale

La boucle infinie n'utilise pas de RTOS. A la place, elle emploie un **scheduler cooperatif** :

- toutes les `20 ms` : bouton
- toutes les `50 ms` : tachymetre
- toutes les `100 ms` : ventilateurs
- toutes les `250 ms` : calcul de regulation
- toutes les `250 ms` : mise a jour temperature simulee
- toutes les `50 ms` : LEDs

Cette approche est simple a lire, simple a deboguer, et tres bien adaptee a un projet labo.

### 2. `Profil.c`

Le module profil prepare les bornes pour chaque ventilateur :

- RPM minimum
- RPM nominal
- RPM maximum
- temperature minimum
- temperature maximum

Il calcule aussi un rapport `gain_num / gain_den`.

Ce rapport sert ensuite a faire une **interpolation lineaire** dans `AFLCalcul.c`.

Autrement dit :

- si la temperature est basse, on garde une petite vitesse
- si la temperature monte, on augmente progressivement
- si la temperature est haute, on va au maximum

### 3. `AFLCalcul.c`

Ce module est le coeur mathematique du projet.

Pour chaque ventilateur :

1. il lit le profil choisi
2. il lit la temperature associee
3. il regarde le retour tachymetre
4. il produit une consigne `target_rpm`

Le calcul se fait en 3 zones :

- sous `temp_min_c` : on garde `min_rpm`
- au-dessus de `temp_max_c` : on force `max_rpm`
- entre les deux : on interpole lineairement

Il y a ensuite une protection pratique :

- si le tach indique que le ventilateur est en retard par rapport a la cible
- on evite de demander une vitesse trop basse
- cela aide a maintenir le ventilateur dans une zone stable

### 4. `tachometer.c`

Le tachymetre mesure la vitesse reelle des ventilateurs.

Le principe :

- `TIM2` capture le temps entre deux fronts du signal tach
- une interruption appelle `Tachometer_OnCapture()`
- la tache periodique `Tachometer_Task()` convertit cette periode en RPM

Le module garde deux valeurs utiles :

- `rpm_instant` : valeur instantanee
- `rpm_filtered` : moyenne glissante plus stable pour l'application

Il gere aussi un **timeout** :

- si aucun front n'est vu pendant trop longtemps
- le signal est considere absent
- le RPM est remis a zero

### 5. `fan_control.c`

Ce module traduit la commande logique en action materielle.

Il sait :

- appliquer un duty PWM a chaque ventilateur
- convertir une cible RPM en duty PWM
- suivre l'etat logique d'un ventilateur
- detecter un echec de demarrage

Les etats possibles sont :

- `OFF`
- `STARTING`
- `RUNNING`
- `ERROR`

Le module garde aussi des informations utiles a l'interface :

- action en cours : acceleration, deceleration, attente, rampe
- alerte : warning ou fault
- pourcentage PWM
- RPM reel

### 6. `ui_lcd.c` et `LCD.c`

`LCD.c` parle directement au LCD.

`ui_lcd.c` decide **quoi afficher** :

- profils prets
- attente de confirmation
- test de demarrage
- fonctionnement normal
- safe state

Le rendu utilise un cache de lignes pour eviter de reecrire inutilement le LCD, ce qui limite le clignotement.

### 7. `uart_cmd.c`

Ce module sert surtout au debug.

Commandes utiles :

- `h` : aide
- `s` : etat systeme
- `t` : etat tachymetre
- `c` : confirme le demarrage
- `0..9` : force le duty des ventilateurs
- `a` : force 100 %
- `z` : lance un test de rampe
- `?` : active/desactive l'ecran debug LCD

## Lecture recommandee si vous voulez comprendre le projet

Ordre conseille :

1. `Core/Src/main.c`
2. `Core/Src/Profil.c`
3. `Core/Src/AFLCalcul.c`
4. `Core/Src/tachometer.c`
5. `Core/Src/fan_control.c`
6. `Core/Src/ui_lcd.c`
7. `Core/Src/uart_cmd.c`

## Ce qu'il faut retenir si on veut reutiliser ce code ailleurs

Les parties les plus reutilisables sont :

- la machine a etats de `main.c`
- le calcul de regulation de `AFLCalcul.c`
- le module profil de `Profil.c`
- la logique de diagnostic de `fan_control.c`
- le module tachymetre de `tachometer.c`

Les parties les moins portables sont :

- `LCD.c` car il depend du cablage exact
- `gpio.c`, `tim.c`, `usart.c` car ils dependent de la carte STM32 choisie

Pour porter ce projet dans un autre code, il faut surtout separer :

- la logique metier portable
- la couche materielle specifique STM32

## Regle simple pour lire chaque module

Presque tous les modules suivent la meme structure :

1. constantes `#define`
2. structures internes `typedef struct`
3. variables statiques globales du module
4. fonctions publiques appelees par `main.c`
5. fonctions privees `static`

Quand vous relisez un fichier, vous pouvez donc vous poser cette question :

`Quelles donnees le module garde-t-il ?`

Puis :

`Quelle fonction publique fait quoi ?`

Puis :

`Quelles fonctions static font le travail interne ?`

Cette methode marche tres bien pour reprendre ce code plus tard ou pour le refactorer.
