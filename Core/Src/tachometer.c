#include "tachometer.h"

// TIM2 capture les periodes des signaux tach.
#define TACH_TIM_TICK_HZ            125000UL
#define TACH_PULSES_PER_REVOLUTION  2UL
#define TACH_TIMEOUT_MS             1200UL
#define TACH_FILTER_SAMPLES         4U

// Etat d'un canal tachymetre.
typedef struct {
    volatile uint32_t last_capture;
    volatile uint32_t period_ticks;
    volatile uint32_t last_edge_ms;
    volatile uint8_t first_capture;
    volatile uint8_t pending_period;
    uint16_t samples[TACH_FILTER_SAMPLES];
    uint8_t sample_count;
    uint8_t sample_index;
    TachometerReading_t reading;
} TachometerChannel_t;

static TachometerChannel_t g_tach[TACHOMETER_CHANNEL_COUNT];

static int32_t Tachometer_ChannelFromHal(const TIM_HandleTypeDef *htim);
static void Tachometer_ResetSamples(TachometerChannel_t *channel);
static uint16_t Tachometer_ComputeAverage(const TachometerChannel_t *channel);

void Tachometer_Init(void)
{
    uint8_t index;

    // Reset complet des 4 canaux.
    for (index = 0U; index < TACHOMETER_CHANNEL_COUNT; index++) {
        g_tach[index].first_capture = 1U;
        g_tach[index].pending_period = 0U;
        Tachometer_ResetSamples(&g_tach[index]);
        g_tach[index].reading.rpm_instant = 0U;
        g_tach[index].reading.rpm_filtered = 0U;
        g_tach[index].reading.signal_present = false;
        g_tach[index].reading.timeout = false;
    }
}

void Tachometer_OnCapture(TIM_HandleTypeDef *htim)
{
    int32_t channel_index;
    TachometerChannel_t *channel;
    uint32_t now_capture;
    uint32_t period_ticks;
    uint32_t tim_channel;

    if ((htim == NULL) || (htim->Instance != TIM2)) {
        return;
    }

    // On convertit le channel HAL en index 0..3.
    channel_index = Tachometer_ChannelFromHal(htim);
    if ((channel_index < 0) || (channel_index >= (int32_t)TACHOMETER_CHANNEL_COUNT)) {
        return;
    }

    channel = &g_tach[channel_index];
    tim_channel = (uint32_t)(TIM_CHANNEL_1 + ((uint32_t)channel_index * 4UL));
    now_capture = HAL_TIM_ReadCapturedValue(htim, tim_channel);

    // Premiere capture : on memorise seulement le point de depart.
    if (channel->first_capture != 0U) {
        channel->last_capture = now_capture;
        channel->last_edge_ms = HAL_GetTick();
        channel->first_capture = 0U;
        channel->reading.signal_present = true;
        channel->reading.timeout = false;
        return;
    }

    if (now_capture >= channel->last_capture) {
        period_ticks = now_capture - channel->last_capture;
    } else {
        // Gestion du overflow 16 bits du timer.
        period_ticks = (0xFFFFUL - channel->last_capture) + now_capture + 1UL;
    }

    channel->last_capture = now_capture;
    channel->last_edge_ms = HAL_GetTick();

    if (period_ticks > 0UL) {
        channel->period_ticks = period_ticks;
        channel->pending_period = 1U;
    }
}

void Tachometer_Task(uint32_t now_ms)
{
    uint8_t index;

    for (index = 0U; index < TACHOMETER_CHANNEL_COUNT; index++) {
        TachometerChannel_t *channel = &g_tach[index];

        if ((channel->first_capture == 0U) && ((now_ms - channel->last_edge_ms) > TACH_TIMEOUT_MS)) {
            channel->reading.rpm_instant = 0U;
            channel->reading.rpm_filtered = 0U;
            channel->reading.signal_present = false;
            channel->reading.timeout = true;
            channel->pending_period = 0U;
            Tachometer_ResetSamples(channel);
            continue;
        }

        // Une nouvelle periode a ete mesuree.
        if (channel->pending_period != 0U) {
            uint32_t rpm;

            channel->pending_period = 0U;

            // rpm = frequence * 60 / pulses_par_tour
            rpm = ((TACH_TIM_TICK_HZ * 60UL) / channel->period_ticks) / TACH_PULSES_PER_REVOLUTION;
            if (rpm > 65535UL) {
                rpm = 65535UL;
            }

            channel->reading.rpm_instant = (uint16_t)rpm;
            channel->samples[channel->sample_index] = (uint16_t)rpm;
            channel->sample_index = (uint8_t)((channel->sample_index + 1U) % TACH_FILTER_SAMPLES);
            if (channel->sample_count < TACH_FILTER_SAMPLES) {
                channel->sample_count++;
            }

            // Petit filtre moyenne glissante.
            channel->reading.rpm_filtered = Tachometer_ComputeAverage(channel);
            channel->reading.signal_present = true;
            channel->reading.timeout = false;
        }
    }
}

const TachometerReading_t *Tachometer_GetReading(uint8_t index)
{
    if (index >= TACHOMETER_CHANNEL_COUNT) {
        return NULL;
    }

    return &g_tach[index].reading;
}

void Tachometer_GetSnapshot(TachometerSnapshot_t *snapshot)
{
    uint8_t index;

    if (snapshot == NULL) {
        return;
    }

    for (index = 0U; index < TACHOMETER_CHANNEL_COUNT; index++) {
        snapshot->fans[index] = g_tach[index].reading;
    }
}

static int32_t Tachometer_ChannelFromHal(const TIM_HandleTypeDef *htim)
{
    switch (htim->Channel) {
        case HAL_TIM_ACTIVE_CHANNEL_1:
            return 0;
        case HAL_TIM_ACTIVE_CHANNEL_2:
            return 1;
        case HAL_TIM_ACTIVE_CHANNEL_3:
            return 2;
        case HAL_TIM_ACTIVE_CHANNEL_4:
            return 3;
        default:
            return -1;
    }
}

static void Tachometer_ResetSamples(TachometerChannel_t *channel)
{
    uint8_t sample;

    // Vide la fenetre du filtre.
    channel->sample_count = 0U;
    channel->sample_index = 0U;

    for (sample = 0U; sample < TACH_FILTER_SAMPLES; sample++) {
        channel->samples[sample] = 0U;
    }
}

static uint16_t Tachometer_ComputeAverage(const TachometerChannel_t *channel)
{
    uint32_t total = 0UL;
    uint8_t sample;

    if (channel->sample_count == 0U) {
        return 0U;
    }

    // Moyenne simple. Suffisant pour lisser un peu le RPM.
    for (sample = 0U; sample < channel->sample_count; sample++) {
        total += channel->samples[sample];
    }

    return (uint16_t)(total / channel->sample_count);
}
