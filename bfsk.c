
#include "bfsk.h"

struct bfsk {
	float sample_rate;
	float zero;
	float one;
	float max_deviation;
	int flags;

	int polarity;
	int period_samples;

	int total_samples;
};

bfsk_t * bfsk_init(float sample_rate, float zero, float one, float max_deviation, int flags) {
	bfsk_t * d = malloc(sizeof(struct bfsk));
	if (d == NULL) {
		return NULL;
	}

	d->sample_rate = sample_rate;
	d->zero = zero;
	d->one = one;
	d->max_deviation = max_deviation;
	d->flags = flags;

	d->polarity = -1;
	d->period_samples = 0;

	d->total_samples = 0;

	return d;
}

bfsk_result_t bfsk_analyze(bfsk_t * d, const float ** samples, size_t * sample_count) {
	bfsk_result_t result = BFSK_END;

	while (*sample_count > 0 && result == BFSK_END) {
		int cur_pol = **samples < 0 ? -1 : +1;
		if (d->flags & BFSK_INVERT_POLARITY) {
			cur_pol = -cur_pol;
		}

		if (d->polarity == -1 && cur_pol == +1) {
			if (d->period_samples > 0) {
				float freq = d->sample_rate / d->period_samples;
				if (freq >= d->zero - d->max_deviation && freq <= d->zero + d->max_deviation) {
					result = BFSK_ZERO;
					//printf("%i %f 0\n", d->total_samples, freq);
				} else if (freq >= d->one - d->max_deviation && freq <= d->one + d->max_deviation) {
					result = BFSK_ONE;
					//printf("%i %f 1\n", d->total_samples, freq);
				} else {
					result = BFSK_INVALID;
					//printf("%i %f R\n", d->total_samples, freq);
				}
			}

			d->period_samples = 1;

		} else if (d->period_samples > 0) {
			d->period_samples++;
		}

		d->polarity = cur_pol;
		d->total_samples++;
		(*samples)++;
		(*sample_count)--;
	}

	return result;
}

void bfsk_free(bfsk_t * d) {
	free(d);
}
