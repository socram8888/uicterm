
// Based on Cypress Semiconductor AN2336 ("PSoC®1 -Simplified FSK Detection")

#include "bfsk.h"
#include <math.h>
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
	float * corr;

	/**
	 * Correlator buffer size
	 */
	size_t corr_size;

	/**
	 * Number of values in {@code corr}
	 */
	size_t corr_count;

	/**
	 * Index of oldest value in correlator outputs buffer
	 */
	size_t corr_idx;

	/**
	 * Last correlator sign: -1 for low frequency, or +1 for high frequency
	 */
	int polarity;

	/**
	 * Number of bits detected with current polarity
	 */
	float pol_bits;
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

	// Initialize by default with a correlation buffer size of 3/5 of bit
	// It's worked fine in my tests
	d->corr_size = (sample_rate * 3) / (d->params.bps * 5);
	d->corr_count = 0;
	d->corr_idx = 0;

	d->corr = malloc(d->corr_size * sizeof(float));
	if (d == NULL) {
		bfsk_free(d);
		return NULL;
	}

	d->polarity = 0;
	d->pol_bits = 0;

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

			// Calculate correlation
			d->corr[d->corr_idx] = sample * prevsample;
			d->corr_idx = (d->corr_idx + 1) % d->corr_size;
			if (d->corr_count < d->corr_size) {
				d->corr_count++;
			}

			// Calculate sum of last correlator outputs
			float corrsum = 0;
			for (size_t i = 0; i < d->corr_count; i++) {
				corrsum += d->corr[i];
			}

			int polarity = corrsum >= 0 ? 1 : -1;
			if (d->polarity == polarity) {
				int old_int = (int) d->pol_bits;
				d->pol_bits += d->params.bps / d->sample_rate;
				int cur_int = (int) d->pol_bits;

				if (old_int < cur_int) {
					if ((polarity == 1) ^ (d->params.mark_hz < d->params.space_hz)) {
						//printf("1");
						result = BFSK_ONE;
					} else {
						//printf("0");
						result = BFSK_ZERO;
					}
				}
			} else {
				if (d->pol_bits < 1) {
					result = BFSK_INVALID;
				}

				d->polarity = polarity;

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
	if (d == NULL) {
		return;
	}

	free(d->prev);
	free(d);
}
