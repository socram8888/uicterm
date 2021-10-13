
// Based on Cypress Semiconductor AN2336 ("PSoC®1 -Simplified FSK Detection")

#include "bfsk.h"
#include <math.h>
#include <stdint.h>
#include <stdbool.h>

struct bfsk {
	/**
	 * BFSK parameters
	 */
	struct bfsk_params params;

	/**
	 * Input sample rate
	 */
	float sample_rate;

	/**
	 * Previous samples buffer
	 */
	int_fast8_t * prev;

	/**
	 * Size, in number of floats, of previous sample buffer
	 */
	size_t prev_size;

	/**
	 * Index of oldest sample in sample buffer
	 */
	size_t prev_idx;

	/**
	 * Last correlator outputs
	 */
	int_fast8_t * corr;

	/**
	 * Correlator buffer size
	 */
	size_t corr_size;

	/**
	 * Index of oldest value in correlator outputs buffer
	 */
	size_t corr_idx;

	/**
	 * Sum of all values in the correlator buffer
	 */
	int_fast32_t corr_sum;

	/**
	 * 1 if output of the correlator should be inverted, else 0.
	 *
	 * The correlator sum's sign indicates if the frequency is leaning towards the lower
	 * frequency (negative sum) or high frequency (positive sum).
	 *
	 * If we assume the space's frequency is lower than the mark's, a negative indicates a
	 * binary 0 and a positive indicates a binary 1.
	 *
	 * However, if the space's frequency is higher than the mark's, the above logic needs to
	 * be inverted. This flag marks that.
	 */
	int_fast8_t invert_corr;

	/**
	 * Last emitted bit, or -1 if none has been emitted yet.
	 */
	int_fast8_t previous_bit;

	/**
	 * Number of consecutive bits emitted since last change.
	 */
	float emitted_bits;

	/**
	 * Bits per sample.
	 */
	float bits_per_sample;
};

bfsk_t * bfsk_init(const struct bfsk_params * params, float sample_rate) {
	bfsk_t * d = malloc(sizeof(struct bfsk));
	if (d == NULL) {
		return NULL;
	}

	d->params = *params;
	d->sample_rate = sample_rate;

	// TODO: this is hardcoded for 1700 and 1300Hz - calculate it properly
	float x = sample_rate * 350.0 / 300000.0;
	d->prev_size = ceil(x) - 1;
	d->prev_idx = 0;

	d->prev = calloc(sizeof(*d->prev), d->prev_size);
	if (d == NULL) {
		bfsk_free(d);
		return NULL;
	}

	// Initialize by default with a correlation buffer size of 6/8 of bit
	// It's worked fine in my tests
	d->corr_size = (sample_rate * 6) / (d->params.bps * 8);
	d->corr_idx = 0;
	d->corr_sum = 0;
	d->invert_corr = d->params.mark_hz < d->params.space_hz;

	d->corr = calloc(sizeof(*d->corr), d->corr_size);
	if (d == NULL) {
		bfsk_free(d);
		return NULL;
	}

	d->previous_bit = -1;
	d->emitted_bits = 0;

	return d;
}

bfsk_result_t bfsk_analyze(bfsk_t * d, const float ** samples, size_t * sample_count) {
	bfsk_result_t result = BFSK_END;

	while (*sample_count > 0 && result == BFSK_END) {
		// Calculate the sign of the current sample
		int_fast8_t sample_sign = **samples >= 0 ? 1 : -1;

		/*
		 * Multiply with the output of the delay.
		 *
		 * We don't need to special case the start since the buffers are all zero,
		 * so the resulting multiplications will be zero.
		 */
		int_fast8_t new_corr_sign = d->prev[d->prev_idx] * sample_sign;

		// Update correlator sum
		int_fast8_t old_corr_sign = d->corr[d->corr_idx];
		d->corr_sum = d->corr_sum - old_corr_sign + new_corr_sign;

		// Overwrite old correlator output
		d->corr[d->corr_idx] = new_corr_sign;
		d->corr_idx = (d->corr_idx + 1) % d->corr_size;

		// Invert if required
		int_fast8_t curr_bit = (d->corr_sum >= 0) ^ d->invert_corr;
		if (curr_bit == d->previous_bit) {
			// Cast to int to floor it
			int_fast32_t old_int = (int_fast32_t) d->emitted_bits;
			d->emitted_bits += d->params.bps / d->sample_rate;
			int_fast32_t cur_int = (int_fast32_t) d->emitted_bits;

			// If we have received a new full bit, feed it
			if (old_int < cur_int) {
				if (d->previous_bit) {
					//printf("1");
					result = BFSK_ONE;
				} else {
					//printf("0");
					result = BFSK_ZERO;
				}
			}
		} else {
			if (d->emitted_bits < 1) {
				result = BFSK_INVALID;
			}

			d->previous_bit = curr_bit;

			// Half bit to sample in the middle
			d->emitted_bits = 0.5;
		}

		// Save this sample
		d->prev[d->prev_idx] = sample_sign;
		d->prev_idx = (d->prev_idx + 1) % d->prev_size;

		(*samples)++;
		(*sample_count)--;
	}

	return result;
}

void bfsk_free(bfsk_t * d) {
	if (d == NULL) {
		return;
	}

	free(d->prev);
	free(d);
}
