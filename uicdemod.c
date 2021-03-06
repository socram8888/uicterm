
#include "uicdemod.h"
#include "goertzel.h"
#include "bfsk.h"
#include "signal.h"

struct uicdemod {
	goertzel_t * goertzel;
	bfsk_t * demod;
	telegram_t * telegram;

	bool ran_goertzel;
	bool has_telegram;

	int last_signal;
	int current_signal;
	int current_signal_ticks;
	int required_ticks;
	float tone_certainty;
};

static const struct bfsk_params fskparams = {
	.bps = 600,
	.mark_hz = 1300,
	.space_hz = 1700
};

static const float freqs[] = {
	1520, // Warning
	1960, // Listening
	2280, // Channel free
	2800  // Pilot
};

#define abs(x) ((x) < 0 ? -(x) : (x))

uicdemod_t * uicdemod_init(float sample_rate) {
	uicdemod_t * d = malloc(sizeof(struct uicdemod));
	if (d == NULL) {
		return NULL;
	}

	d->goertzel = goertzel_init(freqs, 4, sample_rate);
	if (d->goertzel == NULL) {
		uicdemod_free(d);
		return NULL;
	}

	// TODO: configurable deviation
	d->demod = bfsk_init(&fskparams, sample_rate);
	if (d->demod == NULL) {
		uicdemod_free(d);
		return NULL;
	}

	d->telegram = telegram_init();
	if (d->telegram == NULL) {
		uicdemod_free(d);
		return NULL;
	}

	d->last_signal = -1;
	d->current_signal = -1;
	d->required_ticks = 3;
	d->tone_certainty = 0.75;

	return d;
}

void uicdemod_analyze_begin(uicdemod_t * d) {
	d->ran_goertzel = false;
	d->has_telegram = false;
}

uicdemod_status_t uicdemod_analyze(uicdemod_t * d, const float ** samples, size_t * sample_count) {
	uicdemod_status_t status = UICDEMOD_NONE;

	if (d->has_telegram) {
		d->has_telegram = false;
		return UICDEMOD_PACKET;
	}

	if (!d->ran_goertzel) {
		d->ran_goertzel = true;

		// Calculate magnitude for all four frequencies
		float fmag[4];
		goertzel_magnitude(d->goertzel, *samples, *sample_count, fmag);

		// Calculate the signal power
		float signal_power = 0;
		for (size_t i = 0; i < *sample_count; i++) {
			signal_power += abs((*samples)[i]);
		}

		// Get frequency exceeding a certainty level
		int new_signal = 4;
		float new_signal_power = 0;
		for (int i = 0; i < 4; i++) {
			float fmag_norm = fmag[i] / signal_power;
			if (fmag_norm > d->tone_certainty && fmag_norm > new_signal_power) {
				new_signal = i;
				new_signal_power = fmag_norm;
			}
		}

		if (new_signal == d->current_signal) {
			d->current_signal_ticks++;
		} else {
			d->current_signal = new_signal;
			d->current_signal_ticks = 1;
		}

		if (d->last_signal != d->current_signal && d->current_signal_ticks == d->required_ticks) {
			switch (d->current_signal) {
				case 0:
					status = UICDEMOD_WARNING;
					break;
				case 1:
					status = UICDEMOD_LISTENING;
					break;
				case 2:
					status = UICDEMOD_CHFREE;
					break;
				case 3:
					status = UICDEMOD_PILOT;
					break;
				case 4:
					status = UICDEMOD_SILENCE;
			}

			d->last_signal = d->current_signal;
		}
	}

	// Signal 4, aka no signal
	if (1 || d->current_signal == 4) {
		while (status == UICDEMOD_NONE && *sample_count > 0) {
			bfsk_result_t bfskres = bfsk_analyze(d->demod, samples, sample_count);
			int bit;

			switch (bfskres) {
				case BFSK_ZERO:
				case BFSK_ONE:
					bit = (bfskres == BFSK_ONE ? 1 : 0);

					telegram_feed(d->telegram, bit);
					if (telegram_is_done(d->telegram)) {
						// Ensure we always issue a silence before a packet
						if (d->last_signal != 4) {
							status = UICDEMOD_SILENCE;
							d->last_signal = 4;
							d->current_signal = 4;
							d->current_signal_ticks = 1;
							d->has_telegram = true;
						} else {
							status = UICDEMOD_PACKET;
						}
					}

					break;

				case BFSK_INVALID:
					telegram_reset(d->telegram);
					break;

				default:
					break;
			}
		}
	}

	return status;
}

telegram_t * uicdemod_get_telegram(uicdemod_t * d) {
	return d->telegram;
}

void uicdemod_set_required_ticks(uicdemod_t * d, int ticks) {
	d->required_ticks = ticks;
}

void uicdemod_set_tone_certainty(uicdemod_t * d, float threshold) {
	d->tone_certainty = threshold;
}

void uicdemod_free(uicdemod_t * d) {
	if (d == NULL) {
		return;
	}

	telegram_free(d->telegram);
	bfsk_free(d->demod);
	goertzel_free(d->goertzel);
	free(d);
}
