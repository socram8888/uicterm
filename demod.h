
#pragma once
#include <stdlib.h>

typedef struct demod demod_t;

typedef enum {
	DEMOD_END,
	DEMOD_INVALID,
	DEMOD_ZERO,
	DEMOD_ONE
} demod_result_t;

/**
 * Initializes a new zero-crossing demodulator.
 *
 * @param sample_rate Input sample rate
 * @param zero Zero bit frequency, in hertz
 * @param one One bit frequency, in hertz
 * @param max_deviation Frequency variation tolerance, in hertz
 * @returns New demodulator, or NULL on error
 */
demod_t * demod_init(float sample_rate, float zero, float one, float max_deviation);

/**
 * Analizes the input samples and returns the result. Updates sample and
 * sample count.
 *
 * @param d Demodulator object
 * @param samples_ptr Pointer to input samples
 * @param sample_count_ptr Pointer to number of samples
 * @returns a demod_result
 */
demod_result_t demod_analyze(demod_t * d, const float ** samples_ptr, size_t * sample_count_ptr);

/**
 * Destroys a demodulator object.
 *
 * @param d Demodulator object
 */
void demod_free(demod_t * d);
