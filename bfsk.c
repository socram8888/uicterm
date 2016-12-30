
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
	float * prev;

	/**
	 * Size, in number of floats, of previous sample buffer
	 */
	size_t prev_size;

	/**
	 * Number of samples in previous sample buffer
	 */
	size_t prev_count;

	/**
	 * Index of oldest sample in sample buffer
	 */
	size_t prev_idx;

	/**
	 * Weight of last sample in sample buffer
	 */
	float prev_weight;

	/**
	 * Last correlator outputs
	 */
	uint64_t corr;

	/**
	 * Number of last correlator outputs to keep
	 */
	uint8_t corr_size;

	/**
	 * Number of values in {@code corr}
	 */
	uint8_t corr_count;

	/**
	 * Sum of last correlator outputs: negative for low frequency, or positive for high frequency
	 */
	int8_t corr_sum;

	/**
	 * Number of bits detected with current polarity
	 */
	float pol_bits;

	/**
	 * Boolean to avoid spamming with resets
	 */
	bool last_was_reset;
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
	d->prev_size = ceil(x);
	d->prev_count = 0;
	d->prev_idx = 0;
	d->prev_weight = 1 - (d->prev_size - x);

	d->prev = malloc(d->prev_size * sizeof(float));
	if (d == NULL) {
		bfsk_free(d);
		return NULL;
	}

	d->corr = 0;
	d->corr_count = 0;

	// Initialize by default with a correlation buffer size of 4/5 of bit
	// It's worked fine in my tests
	d->corr_size = (sample_rate * 4) / (d->params.bps * 5);

	// Limit to 63 (64 minus the old bit)
	if (d->corr_size > 63) {
		d->corr_size = 63;
	}

	d->corr_sum = 0;
	d->pol_bits = 0;
	d->last_was_reset = false;

	return d;
}

bfsk_result_t bfsk_analyze(bfsk_t * d, const float ** samples, size_t * sample_count) {
	bfsk_result_t result = BFSK_END;

	while (*sample_count > 0 && result == BFSK_END) {
		float sample = **samples;

		if (d->prev_count == d->prev_size) {
			// Calculate index of the sample before last
			size_t previdx = (d->prev_idx + 1) % d->prev_count;

			// Linearly interpolate previous sample
			float prevsample =
					d->prev[d->prev_idx] * d->prev_weight +
					d->prev[previdx] * (1 - d->prev_weight);
/*
			printf("I ");
			printf("prev[%i] * %f + prev[%i] * %f ", d->prev_idx, d->prev_weight, previdx, 1 - d->prev_weight);
			printf("= %f * %f + %f * %f ", d->prev[d->prev_idx], d->prev_weight, d->prev[previdx], 1 - d->prev_weight);
			printf("= %f\n", prevsample);
*/

			bool was_positive = d->corr_sum >= 0;

			// Calculate correlation, then adjust the sum and save so we can de-adjust later
			if (sample * prevsample >= 0) {
				d->corr_sum++;
				d->corr = d->corr << 1 | 1;
			} else {
				d->corr_sum--;
				d->corr = d->corr << 1 | 0;
			}

			if (d->corr_count == d->corr_size) {
				// Undo the addition/subtraction
				if (d->corr & (1 << d->corr_size)) {
					d->corr_sum--;
				} else {
					d->corr_sum++;
				}
			} else {
				d->corr_count++;
			}

			if (was_positive == (d->corr_sum >= 0)) {
				// If the sign hasn't changed, then increment bit count
				int old_int = (int) d->pol_bits;
				d->pol_bits += d->params.bps / d->sample_rate;
				int cur_int = (int) d->pol_bits;

				// If the integer bit has changed, emit a new bit
				if (old_int < cur_int) {
					if (was_positive ^ (d->params.mark_hz < d->params.space_hz)) {
						//printf("1");
						result = BFSK_ONE;
						d->last_was_reset = false;
					} else {
						//printf("0");
						result = BFSK_ZERO;
						d->last_was_reset = false;
					}
				}
			} else {
				if (d->pol_bits < 1 && !d->last_was_reset) {
					result = BFSK_INVALID;
					d->last_was_reset = true;
				}

				// Half bit to sample in the middle
				d->pol_bits = 0.5;
			}
		} else {
			d->prev_count++;
		}

		// Save this sample
		d->prev[d->prev_idx] = sample;
		d->prev_idx = (d->prev_idx + 1) % d->prev_size;

		(*samples)++;
		(*sample_count)--;
	}

	return result;
}

void bfsk_free(bfsk_t * d) {
	free(d);
}
