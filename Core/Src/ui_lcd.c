#include "ui_lcd.h"

#include "LCD.h"

#include <stdio.h>
#include <string.h>

/*
 * ui_lcd.c
 * --------
 * Couche de presentation du projet sur LCD.
 *
 * LCD.c sait parler au materiel.
 * ui_lcd.c sait quoi afficher selon l'etat de l'application.
 */

// On limite le rafraichissement pour garder un LCD stable.
#define UI_LCD_REFRESH_MS     250UL
#define UI_LCD_PROFILE_PAGE_MS 1500UL

// Liste des ecrans applicatifs.
typedef enum {
    UI_SCREEN_NONE = 0,
    UI_SCREEN_PROFILES_READY,
    UI_SCREEN_WAIT_CONFIRM,
    UI_SCREEN_STARTUP_TEST,
    UI_SCREEN_RUNTIME,
    UI_SCREEN_SAFE_STATE
} UI_LCD_Screen_t;

// Contexte complet du LCD.
typedef struct {
    UI_LCD_Screen_t screen;
    uint32_t next_refresh_ms;
    uint32_t page_started_ms;
    uint8_t page_index;
    uint8_t dirty;
    uint8_t passed_mask;
    uint8_t fail_mask;
    uint8_t complete;
    uint8_t debug_overlay;
    const Profil_Result_t *profiles;
    const TemperatureSnapshot_t *temps;
    const AFLCalcul_Output_t *targets;
    const FanStatus_t *fans;
    char cache[4][21];
} UI_LCD_Context_t;

static UI_LCD_Context_t g_ui;

static void UI_LCD_ClearCache(void);
static void UI_LCD_WriteLine(uint8_t row, const char *text);
static void UI_LCD_Render(void);
static void UI_LCD_RenderProfileScreen(const char *header);
static void UI_LCD_RenderStartupTest(void);
static void UI_LCD_RenderRuntime(void);
static void UI_LCD_RenderFanDebug(void);
static uint8_t UI_LCD_GetProfilePage(void);
static const char *UI_LCD_StateShort(FanState_t state);

void UI_LCD_Init(void)
{
    LCD_Init();
    LCD_Clear();
    UI_LCD_ClearCache();

    // Ecran vide au boot.
    g_ui.screen = UI_SCREEN_NONE;
    g_ui.next_refresh_ms = HAL_GetTick();
    g_ui.page_started_ms = HAL_GetTick();
    g_ui.page_index = 0U;
    g_ui.dirty = 1U;
    g_ui.debug_overlay = 0U;
}

void UI_LCD_Task(uint32_t now_ms)
{
    // Si rien n'a change et qu'on est avant le prochain refresh, on sort.
    if ((now_ms < g_ui.next_refresh_ms) && (g_ui.dirty == 0U)) {
        return;
    }

    // On alterne entre page 1 et page 2 pour afficher 4 fans sur 20x4.
    // Le LCD n'a pas assez de place pour tout a la fois.
    if ((g_ui.screen == UI_SCREEN_PROFILES_READY) || (g_ui.screen == UI_SCREEN_WAIT_CONFIRM) || (g_ui.screen == UI_SCREEN_RUNTIME) || (g_ui.debug_overlay != 0U)) {
        if ((now_ms - g_ui.page_started_ms) >= UI_LCD_PROFILE_PAGE_MS) {
            g_ui.page_started_ms = now_ms;
            g_ui.page_index ^= 1U;
            g_ui.dirty = 1U;
        }
    }

    g_ui.next_refresh_ms = now_ms + UI_LCD_REFRESH_MS;
    UI_LCD_Render();
    g_ui.dirty = 0U;
}

void UI_LCD_ShowProfilesReady(const Profil_Result_t *profiles)
{
    if (g_ui.screen != UI_SCREEN_PROFILES_READY) {
        g_ui.page_started_ms = HAL_GetTick();
        g_ui.page_index = 0U;
    }
    g_ui.screen = UI_SCREEN_PROFILES_READY;
    g_ui.profiles = profiles;
    g_ui.dirty = 1U;
}

void UI_LCD_ShowWaitConfirm(const Profil_Result_t *profiles)
{
    if (g_ui.screen != UI_SCREEN_WAIT_CONFIRM) {
        g_ui.page_started_ms = HAL_GetTick();
        g_ui.page_index = 0U;
    }
    g_ui.screen = UI_SCREEN_WAIT_CONFIRM;
    g_ui.profiles = profiles;
    g_ui.dirty = 1U;
}

void UI_LCD_ShowStartupTest(uint8_t passed_mask, uint8_t fail_mask, uint8_t complete)
{
    g_ui.screen = UI_SCREEN_STARTUP_TEST;
    g_ui.passed_mask = passed_mask;
    g_ui.fail_mask = fail_mask;
    g_ui.complete = complete;
    g_ui.dirty = 1U;
}

void UI_LCD_ShowRuntime(const Profil_Result_t *profiles,
                        const TemperatureSnapshot_t *temps,
                        const AFLCalcul_Output_t *targets,
                        const FanStatus_t fans[FAN_CONTROL_CHANNEL_COUNT])
{
    if (g_ui.screen != UI_SCREEN_RUNTIME) {
        g_ui.page_started_ms = HAL_GetTick();
        g_ui.page_index = 0U;
    }
    g_ui.screen = UI_SCREEN_RUNTIME;
    g_ui.profiles = profiles;
    g_ui.temps = temps;
    g_ui.targets = targets;
    g_ui.fans = fans;
    g_ui.dirty = 1U;
}

void UI_LCD_ShowSafeState(uint8_t fail_mask)
{
    g_ui.screen = UI_SCREEN_SAFE_STATE;
    g_ui.fail_mask = fail_mask;
    g_ui.dirty = 1U;
}

void UI_LCD_ToggleFanDebug(void)
{
    g_ui.debug_overlay ^= 1U;
    g_ui.page_started_ms = HAL_GetTick();
    g_ui.page_index = 0U;
    g_ui.dirty = 1U;
}

static void UI_LCD_ClearCache(void)
{
    uint8_t row;

    // Le cache evite de reecrire la meme ligne.
    for (row = 0U; row < 4U; row++) {
        memset(g_ui.cache[row], 0, sizeof(g_ui.cache[row]));
    }
}

static void UI_LCD_WriteLine(uint8_t row, const char *text)
{
    char fixed[21];
    size_t len;

    if ((row > 3U) || (text == NULL)) {
        return;
    }

    // Le LCD est 20 colonnes.
    // On tronque ou on remplit avec des espaces.
    memset(fixed, ' ', 20U);
    fixed[20] = '\0';
    len = strlen(text);
    if (len > 20U) {
        len = 20U;
    }
    memcpy(fixed, text, len);

    if (strncmp(g_ui.cache[row], fixed, 20U) != 0) {
        // On evite de reecrire une ligne identique pour limiter le clignotement.
        LCD_WriteAt(0, row, fixed);
        memcpy(g_ui.cache[row], fixed, sizeof(fixed));
    }
}

static void UI_LCD_Render(void)
{
    // Routeur principal d'ecran.
    if (g_ui.debug_overlay != 0U) {
        UI_LCD_RenderFanDebug();
        return;
    }

    switch (g_ui.screen) {
        case UI_SCREEN_PROFILES_READY:
            UI_LCD_RenderProfileScreen("Profiles Ready");
            break;
        case UI_SCREEN_WAIT_CONFIRM:
            UI_LCD_RenderProfileScreen("Press 'c' UART");
            break;
        case UI_SCREEN_STARTUP_TEST:
            UI_LCD_RenderStartupTest();
            break;
        case UI_SCREEN_RUNTIME:
            UI_LCD_RenderRuntime();
            break;
        case UI_SCREEN_SAFE_STATE:
            UI_LCD_WriteLine(0U, "Startup Failure");
            UI_LCD_WriteLine(1U, "Safe State Active");
            {
                char line[21];
                snprintf(line, sizeof(line), "Fail mask: 0x%02X", g_ui.fail_mask);
                UI_LCD_WriteLine(2U, line);
            }
            UI_LCD_WriteLine(3U, "Reset required");
            break;
        case UI_SCREEN_NONE:
        default:
            UI_LCD_WriteLine(0U, "AFLC Initializing");
            UI_LCD_WriteLine(1U, "");
            UI_LCD_WriteLine(2U, "");
            UI_LCD_WriteLine(3U, "");
            break;
    }
}

static void UI_LCD_RenderProfileScreen(const char *header)
{
    uint8_t page = UI_LCD_GetProfilePage();
    uint8_t first_index = (uint8_t)(page * 2U);
    uint8_t row;

    // Une page montre 2 ventilateurs.
    UI_LCD_WriteLine(0U, header);

    for (row = 0U; row < 2U; row++) {
        char line[21];
        uint8_t fan_index = (uint8_t)(first_index + row);

        if ((g_ui.profiles != NULL) && (fan_index < PROFIL_FAN_COUNT)) {
            snprintf(line,
                     sizeof(line),
                     "F%u %-6s %4ur",
                     (unsigned int)(fan_index + 1U),
                     Profil_GetName(g_ui.profiles->fans[fan_index].profile_id),
                     (unsigned int)g_ui.profiles->fans[fan_index].nominal_rpm);
        } else {
            snprintf(line, sizeof(line), "F%u ----", (unsigned int)(fan_index + 1U));
        }

        UI_LCD_WriteLine((uint8_t)(row + 1U), line);
    }

    if (g_ui.screen == UI_SCREEN_WAIT_CONFIRM) {
        UI_LCD_WriteLine(3U, "Waiting confirm...");
    } else {
        char footer[21];
        snprintf(footer, sizeof(footer), "Page %u/2", (unsigned int)(page + 1U));
        UI_LCD_WriteLine(3U, footer);
    }
}

static void UI_LCD_RenderStartupTest(void)
{
    uint8_t index;
    char summary[21];

    // Deux premieres lignes : F1/F2.
    // Derniere ligne : resume F3/F4.
    UI_LCD_WriteLine(0U, g_ui.complete ? "Startup Test Done" : "Startup Test");

    for (index = 0U; index < 2U; index++) {
        char line[21];
        uint8_t fan_index = index;
        char status = '-';

        if ((g_ui.fail_mask & (1U << fan_index)) != 0U) {
            status = 'F';
        } else if ((g_ui.passed_mask & (1U << fan_index)) != 0U) {
            status = 'P';
        }

        snprintf(line, sizeof(line), "Fan%u Tach [%c]", (unsigned int)(fan_index + 1U), status);
        UI_LCD_WriteLine((uint8_t)(index + 1U), line);
    }

    snprintf(summary,
             sizeof(summary),
             "F3[%c] F4[%c]",
             ((g_ui.fail_mask & (1U << 2U)) != 0U) ? 'F' : (((g_ui.passed_mask & (1U << 2U)) != 0U) ? 'P' : '-'),
             ((g_ui.fail_mask & (1U << 3U)) != 0U) ? 'F' : (((g_ui.passed_mask & (1U << 3U)) != 0U) ? 'P' : '-'));
    UI_LCD_WriteLine(3U, summary);
}

static void UI_LCD_RenderRuntime(void)
{
    uint8_t page = UI_LCD_GetProfilePage();
    uint8_t first_index = (uint8_t)(page * 2U);
    uint8_t row;

    // Pour chaque fan :
    // ligne 1/2 = RPM reel vs cible
    // ligne 3/4 = temperature et duty
    for (row = 0U; row < 2U; row++) {
        char line[21];
        uint8_t fan_index = (uint8_t)(first_index + row);

        if ((g_ui.profiles != NULL) && (g_ui.temps != NULL) && (g_ui.targets != NULL) && (g_ui.fans != NULL) && (fan_index < FAN_CONTROL_CHANNEL_COUNT)) {
            // Ligne 1/2 : reel contre cible.
            snprintf(line,
                     sizeof(line),
                     "F%u %s %u/%u",
                     (unsigned int)(fan_index + 1U),
                     Profil_GetName(g_ui.profiles->fans[fan_index].profile_id),
                     (unsigned int)g_ui.fans[fan_index].rpm,
                     (unsigned int)g_ui.targets->target_rpm[fan_index]);
            UI_LCD_WriteLine(row, line);

            // Ligne 3/4 : contexte thermique et effort PWM.
            snprintf(line,
                     sizeof(line),
                     "T%u=%uC D=%u%%",
                     (unsigned int)(fan_index + 1U),
                     (unsigned int)g_ui.temps->values_c[fan_index],
                     (unsigned int)g_ui.fans[fan_index].percent);
            UI_LCD_WriteLine((uint8_t)(row + 2U), line);
        }
    }
}

static void UI_LCD_RenderFanDebug(void)
{
    uint8_t page = UI_LCD_GetProfilePage();
    uint8_t first_index = (uint8_t)(page * 2U);
    uint8_t row;

    UI_LCD_WriteLine(0U, "Fan Debug");

    for (row = 0U; row < 2U; row++) {
        char line[21];
        uint8_t fan_index = (uint8_t)(first_index + row);

        if ((g_ui.fans != NULL) && (fan_index < FAN_CONTROL_CHANNEL_COUNT)) {
            snprintf(line,
                     sizeof(line),
                     "F%u %s P%3u T%4u",
                     (unsigned int)(fan_index + 1U),
                     UI_LCD_StateShort(g_ui.fans[fan_index].state),
                     (unsigned int)g_ui.fans[fan_index].percent,
                     (unsigned int)g_ui.fans[fan_index].rpm);
        } else {
            snprintf(line, sizeof(line), "F%u ----", (unsigned int)(fan_index + 1U));
        }

        UI_LCD_WriteLine((uint8_t)(row + 1U), line);
    }

    UI_LCD_WriteLine(3U, page == 0U ? "F1/F2  ?=exit" : "F3/F4  ?=exit");
}

static uint8_t UI_LCD_GetProfilePage(void)
{
    // 0 = F1/F2, 1 = F3/F4.
    return g_ui.page_index;
}

static const char *UI_LCD_StateShort(FanState_t state)
{
    switch (state) {
        case FAN_STATE_OFF:
            return "OFF";
        case FAN_STATE_STARTING:
            return "STA";
        case FAN_STATE_RUNNING:
            return "RUN";
        case FAN_STATE_ERROR:
        default:
            return "ERR";
    }
}
