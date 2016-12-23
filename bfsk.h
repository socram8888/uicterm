
#pragma once
#include <stdlib.h>

typedef struct bfsk bfsk_t;

typedef enum {
	BFSK_END,
	BFSK_INVALID,
	BFSK_ZERO,
	BFSK_ONE
} bfsk_result_t;

/**
 * Initializes a new zero-crossing BFSK demodulator.
 *
 * @param sample_rate Input sample rate
 * @param zero Zero bit frequency, in hertz
 * @param one One bit frequency, in hertz
 * @param max_deviation Frequency variation tolerance, in hertz
 * @returns New demodulator, or NULL on error
 */
bfsk_t * bfsk_init(float sample_rate, float zero, float one, float max_deviation);

/**
 * Analizes the input samples and returns the result. Updates sample and
 * sample count.
 *
 * @param d Demodulator object
 * @param samples_ptr Pointer to input samples
 * @param sample_count_ptr Pointer to number of samples
 * @returns a bfsk_result
 */
bfsk_result_t bfsk_analyze(bfsk_t * d, const float ** samples_ptr, size_t * sample_count_ptr);

/**
 * Destroys a demodulator object. Accepts NULL.
 *
 * @param d Demodulator object
 */
void bfsk_free(bfsk_t * d);
