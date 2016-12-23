
#include "uicdemod.h"
#include "goertzel.h"
#include "bfsk.h"
#include "telegram.h"
#include "signal.h"

struct uicdemod {
	goertzel_t * goertzel;
	bfsk_t * demod;
	telegram_t * telegram;

	bool ran_goertzel;
	int last_signal;
	int current_signal;
	int current_signal_ticks;
};

static const float freqs[] = {
	1520, // Warning
	1960, // Listening
	2280, // Channel free
	2800  // Pilot
};

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
	d->demod = bfsk_init(sample_rate, 1700, 1300, 200, BFSK_INVERT_POLARITY);
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

	return d;
}

void uicdemod_analyze_begin(uicdemod_t * d) {
	d->ran_goertzel = false;
}

uicdemod_status_t uicdemod_analyze(uicdemod_t * d, const float ** samples, size_t * sample_count) {
	uicdemod_status_t status = UICDEMOD_NONE;

	if (!d->ran_goertzel) {
		d->ran_goertzel = true;

		// Calculate magnitude for all four frequencies
		float fmag[4];
		goertzel_magnitude(d->goertzel, *samples, *sample_count, fmag);

		// Normalize magnitudes
		normalize(fmag, fmag, 4);

		// Get frequency exceeding a certainty level
		int new_signal;
		for (new_signal = 0; new_signal < 4; new_signal++) {
			// TODO: configurable certainty
			if (fmag[new_signal] > 0.9) {
				break;
			}
		}

		if (new_signal == d->current_signal) {
			d->current_signal_ticks++;
		} else {
			d->current_signal = new_signal;
			d->current_signal_ticks = 1;
		}

		// TODO: configurable window size
		if (d->last_signal != d->current_signal && d->current_signal_ticks == 2) {
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
	if (d->current_signal == 4) {
		while (status == UICDEMOD_NONE && *sample_count > 0) {
			bfsk_result_t bfskres = bfsk_analyze(d->demod, samples, sample_count);
			int bit;

			switch (bfskres) {
				case BFSK_ZERO:
				case BFSK_ONE:
					bit = (bfskres == BFSK_ONE ? 1 : 0);

					if (telegram_feed(d->telegram, bit) == TELEGRAM_OK) {
						status = UICDEMOD_PACKET;
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

void uicdemod_free(uicdemod_t * d) {
	if (d == NULL) {
		return;
	}

	telegram_free(d->telegram);
	bfsk_free(d->demod);
	goertzel_free(d->goertzel);
	free(d);
}
