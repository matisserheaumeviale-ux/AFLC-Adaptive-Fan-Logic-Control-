#include "LCD_UI.h"

#include "LCD.h"

#include <stdio.h>
#include <string.h>

// Petit runtime pour les animations texte.
static UI_Anim_t g_ui_anim = {0U, 0U};
static char g_line_cache[4][21];
static uint8_t g_line_valid[4];

static void UI_FormatFanLine(char out[21], uint8_t fan_index, const FanStatus_t *fan, uint8_t animated);
static void UI_WriteLineCached(uint8_t row, const char *line);

const char* UI_FanStateToStr(FanState_t state){
    // Retourne un texte court fixe pour garder l'alignement LCD.
    switch(state){
        case FAN_STATE_OFF:     return "Off ";
        case FAN_STATE_STARTING:return "Strt";
        case FAN_STATE_RUNNING: return "Run ";
        case FAN_STATE_ERROR:   return "Err.";
        default:                return "Err.";
    }
}

const char* UI_FanActionToStr(FanAction_t action){
    // Symboles visuels tres compacts.
    switch(action){
        case FAN_ACTION_ACCEL:    return "+++";
        case FAN_ACTION_DETECT:   return "@@@";
        case FAN_ACTION_DECEL:    return "---";
        case FAN_ACTION_IDLE:     return "===";
        case FAN_ACTION_RAMP:     return "^^^";
        case FAN_ACTION_NONE:     return "xxx";
        default:                  return "xxx";
    }
}

const char* UI_FanAlertToStr(FanAlert_t alert){
    switch(alert){
        case FAN_ALERT_WARN:     return "!!!";
        case FAN_ALERT_FAULT:    return "!!!";
        case FAN_ALERT_NONE:     return "   ";
        default:                 return "   ";
    }
}

const char* UI_GetAnimatedAction(FanAction_t action){
    static const char *accel_frames[3] = {"+  ", "++ ", "+++"};
    static const char *decel_frames[3] = {"---", " --", "  -"};

    // On anime juste la queue de ligne.
    // Le but est de garder le reste stable.
    switch(action){
        case FAN_ACTION_ACCEL:
            return accel_frames[g_ui_anim.tick % 3U];
        case FAN_ACTION_DETECT:
            return g_ui_anim.blink ? "@@@" : "   ";
        case FAN_ACTION_DECEL:
            return decel_frames[g_ui_anim.tick % 3U];
        case FAN_ACTION_IDLE:
            return g_ui_anim.blink ? "===" : "= =";
        case FAN_ACTION_RAMP:
            return g_ui_anim.blink ? "^^^" : " ^ ";
        case FAN_ACTION_NONE:
        default:
            return "xxx";
    }
}

const char* UI_GetAnimatedAlert(FanAlert_t alert){
    if(alert == FAN_ALERT_NONE){
        return "   ";
    }

    return g_ui_anim.blink ? "!!!" : "   ";
}

void UI_AnimUpdate(void){
    // tick sert aux frames.
    // blink sert aux clignotements binaires.
    g_ui_anim.tick++;
    g_ui_anim.blink ^= 1U;
}

void UI_DrawFanLine(uint8_t row, uint8_t fan_index, const FanStatus_t *fan){
    char line[21];

    if((fan == NULL) || (row > 3U)) return;

    UI_FormatFanLine(line, fan_index, fan, 0U);
    UI_WriteLineCached(row, line);
}

void UI_DrawFanLineAnimated(uint8_t row, uint8_t fan_index, const FanStatus_t *fan){
    char line[21];

    if((fan == NULL) || (row > 3U)) return;

    UI_FormatFanLine(line, fan_index, fan, 1U);
    UI_WriteLineCached(row, line);
}

void UI_DrawAllFans(const FanStatus_t fans[4]){
    uint8_t i;

    if(fans == NULL) return;

    for(i = 0U; i < 4U; i++){
        UI_DrawFanLineAnimated(i, (uint8_t)(i + 1U), &fans[i]);
    }
}

static void UI_FormatFanLine(char out[21], uint8_t fan_index, const FanStatus_t *fan, uint8_t animated){
    const char *state;
    const char *tail;
    uint16_t rpm;
    uint8_t percent;

    // On borne l'affichage pour ne jamais casser le format 20 colonnes.
    state = UI_FanStateToStr(fan->state);
    rpm = (fan->rpm > 9999U) ? 9999U : fan->rpm;
    percent = (fan->percent > 99U) ? 99U : fan->percent;

    if(fan->alert == FAN_ALERT_NONE){
        tail = (animated != 0U) ? UI_GetAnimatedAction(fan->action) : UI_FanActionToStr(fan->action);
    } else {
        tail = (animated != 0U) ? UI_GetAnimatedAlert(fan->alert) : UI_FanAlertToStr(fan->alert);
    }

    (void)snprintf(out, 21, "F%1u %-4s%4u(%02u)%3s",
                   (unsigned int)fan_index,
                   state,
                   (unsigned int)rpm,
                   (unsigned int)percent,
                   tail);
}

static void UI_WriteLineCached(uint8_t row, const char *line){
    char fixed[21];
    size_t len;

    if((row > 3U) || (line == NULL)) return;

    (void)memset(fixed, ' ', 20U);
    fixed[20] = '\0';

    len = strlen(line);
    if(len > 20U){
        len = 20U;
    }

    (void)memcpy(fixed, line, len);

    // Re-ecrire le LCD inutilement fait clignoter.
    // Donc on compare avant d'envoyer.
    if((g_line_valid[row] == 0U) || (strncmp(g_line_cache[row], fixed, 20) != 0)){
        LCD_WriteAt(0, row, fixed);
        (void)memcpy(g_line_cache[row], fixed, sizeof(fixed));
        g_line_valid[row] = 1U;
    }
}
