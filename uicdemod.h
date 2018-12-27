
#pragma once
#include <stdlib.h>
#include "telegram.h"

typedef struct uicdemod uicdemod_t;

typedef enum {
	UICDEMOD_NONE,
	UICDEMOD_WARNING,
	UICDEMOD_LISTENING,
	UICDEMOD_CHFREE,
	UICDEMOD_PILOT,
	UICDEMOD_SILENCE,
	UICDEMOD_PACKET
} uicdemod_status_t;

/**
 * Initializes a new UIC-751-3 demodulator.
 *
 * @param sample_rate Input sample rate
 * @returns New demodulator, or NULL on error
 */
uicdemod_t * uicdemod_init(float sample_rate);

/**
 * Begins a new sample chunk analysis.
 *
 * @param d UIC-751-3 demodulator
 */
void uicdemod_analyze_begin(uicdemod_t * d);

/**
 * Analizes the input samples and returns the result. Updates sample and
 * sample count. Should be called until UICDEMOD_NONE is returned.
 *
 * @param d UIC-751-3 demodulator
 * @param samples Pointer to input samples
 * @param sample_count Pointer to number of samples
 * @returns detected event, or UICDEMOD_NONE if none
 */
uicdemod_status_t uicdemod_analyze(uicdemod_t * d, const float ** samples, size_t * sample_count);

/**
 * Retrieves latest read telegram. Should be accessed right after
 * {@code uicdemod_analyze} returns {@code UICDEMOD_PACKET}.
 */
telegram_t * uicdemod_get_telegram(uicdemod_t * d);

/**
 * Sets the required normalized value of a signal in the Goertzel filter to
 * be considered as present.
 *
 * @param d UIC-751-3 demodulator
 * @param threshold signal certainty
 */
void uicdemod_set_tone_certainty(uicdemod_t * d, float threshold);

/**
 * Sets the required number of consecutive buffers matching a certain signal
 * for a tone signal to be valid.
 *
 * @param d UIC-751-3 demodulator
 * @param ticks number of ticks
 */
void uicdemod_set_required_ticks(uicdemod_t * d, int ticks);

/**
 * Destroys a demodulator object. Accepts NULL.
 *
 * @param d UIC-751-3 demodulator
 */
void uicdemod_free(uicdemod_t * d);
