
#include <stdio.h>
#include <stdint.h>
#include "goertzel.h"
#include "demod.h"
#include "signal.h"

#define SAMPLE_RATE 37500
#define SAMPLE_COUNT SAMPLE_RATE / 20

// From http://stackoverflow.com/a/7924459
#ifdef _WIN32
#	include <fcntl.h>
#	include <io.h>
#endif

static const float freqs[] = {
	1520, // Warning
	1960, // Listening
	2280, // Channel free
	2800  // Pilot
};

static char * signals[] = {
	"Warning",
	"Listening",
	"Free",
	"Pilot",
	"Noise"
};

int main() {
	#ifdef _WIN32
		_setmode(_fileno(stdin), _O_BINARY);
	#endif

	goertzel_t * g = goertzel_init(freqs, 4, SAMPLE_RATE);
	if (!g) {
		perror("goertzel_init");
		return 1;
	}

	int16_t sample_ints[SAMPLE_COUNT];
	float timestamp = 0;

	int current_signal = 4;
	int current_signal_ticks = 0;
	float current_signal_start = 0;

	int printed_signal = -1;

	while (fread(sample_ints, sizeof(int16_t), SAMPLE_COUNT, stdin) == SAMPLE_COUNT) {
		float samples[SAMPLE_COUNT];
		int16_to_float(sample_ints, samples, SAMPLE_COUNT);

		float power[4];
		goertzel_magnitude(g, samples, SAMPLE_COUNT, power);

		normalize(power, power, 4);

		int new_signal;
		for (new_signal = 0; new_signal < 4; new_signal++) {
			if (power[new_signal] > 0.9) {
				break;
			}
		}

		if (new_signal == current_signal) {
			current_signal_ticks++;
		} else {
			current_signal = new_signal;
			current_signal_ticks = 1;
			current_signal_start = timestamp;
		}

		timestamp += 0.05;

		if (current_signal != printed_signal && current_signal_ticks == 2) {
			printf("%02.1f %s\n", current_signal_start, signals[current_signal]);
			printed_signal = current_signal;
		}
	}

	goertzel_free(g);

	if (!feof(stdin)) {
		perror("fread");
		return 1;
	}

	return 0;
}
	