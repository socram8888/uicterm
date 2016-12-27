
#pragma once
#include <stdlib.h>

typedef struct bfsk bfsk_t;

struct bfsk_params {
	float bps;
	float space_hz;
	float mark_hz;
};

typedef enum {
	BFSK_END,
	BFSK_INVALID,
	BFSK_ZERO,
	BFSK_ONE
} bfsk_result_t;

/**
 * Initializes a new correlator-based BFSK demodulator.
 *
 * @param params BFSK modulation params
 * @param sample_rate Input sample rate
 * @returns New demodulator, or NULL on error
 */
bfsk_t * bfsk_init(const struct bfsk_params * params, float sample_rate);

/**
 * Analizes the input samples and returns the result. Updates sample and
 * sample count.
 *
 * @param d Demodulator object
 * @param samples Pointer to input samples
 * @param sample_count Pointer to number of samples
 * @returns a bfsk_result
 */
bfsk_result_t bfsk_analyze(bfsk_t * d, const float ** samples, size_t * sample_count);

/**
 * Sets window size for correlator output.
 *
 * Rather than using the correlator output as is, this implementation smooths
 * its output by using last {@code win_size} correlator outputs.
 *
 * @param d Demodulator object
 * @param win_size Window size
 */
void bfsk_set_window_size(bfsk_t * d, size_t win_size);

/**
 * Destroys a demodulator object. Accepts NULL.
 *
 * @param d Demodulator object
 */
void bfsk_free(bfsk_t * d);
