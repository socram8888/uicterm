
#include "demod.h"

struct demod {
	float sample_rate;
	float zero;
	float one;
	float max_deviation;

	int polarity;
	int period_samples;
};

demod_t * demod_init(float sample_rate, float zero, float one, float max_deviation) {
	demod_t * d = malloc(sizeof(struct demod));
	if (d == NULL) {
		return NULL;
	}

	d->sample_rate = sample_rate;
	d->zero = zero;
	d->one = one;
	d->max_deviation = max_deviation;

	d->polarity = -1;
	d->period_samples = 0;

	return d;
}

demod_result_t demod_analyze(demod_t * d, const float ** samples, size_t * sample_count) {
	demod_result_t result = DEMOD_END;

	while (*sample_count > 0 && result == DEMOD_END) {
		int cur_pol = **samples < 0 ? -1 : +1;

		if (d->polarity == -1 && cur_pol == +1) {
			if (d->period_samples > 0) {
				float freq = d->sample_rate / d->period_samples;
				if (freq >= d->zero - d->max_deviation && freq <= d->zero + d->max_deviation) {
					result = DEMOD_ZERO;
				} else if (freq >= d->one - d->max_deviation && freq <= d->one + d->max_deviation) {
					result = DEMOD_ONE;
				} else {
					result = DEMOD_INVALID;
				}
			}

			d->period_samples = 1;

		} else if (d->period_samples > 0) {
			d->period_samples++;
		}

		d->polarity = cur_pol;
		(*samples)++;
		(*sample_count)--;
	}

	return result;
}

void demod_free(demod_t * d) {
	free(d);
}
