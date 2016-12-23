
#include <stdio.h>
#include <stdint.h>
#include "uicdemod.h"
#include "signal.h"

#define SAMPLE_RATE 37500
#define SAMPLE_COUNT SAMPLE_RATE / 20

// From http://stackoverflow.com/a/7924459
#ifdef _WIN32
#	include <fcntl.h>
#	include <io.h>
#endif

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

	uicdemod_t * uic = uicdemod_init(SAMPLE_RATE);
	if (!uic) {
		perror("uicdemod_init");
		return 1;
	}

	int16_t sample_ints[SAMPLE_COUNT];
	float timestamp = 0;

	while (fread(sample_ints, sizeof(int16_t), SAMPLE_COUNT, stdin) == SAMPLE_COUNT) {
		float samples[SAMPLE_COUNT];
		int16_to_float(sample_ints, samples, SAMPLE_COUNT);

		uicdemod_analyze_begin(uic);

		const float * sample_ptr = samples;
		size_t sample_count = SAMPLE_COUNT;
		uicdemod_status_t event = uicdemod_analyze(uic, &sample_ptr, &sample_count);
		while (event != UICDEMOD_NONE) {
			printf("EVENT %i\n", event);

			event = uicdemod_analyze(uic, &sample_ptr, &sample_count);
		}
	}

	uicdemod_free(uic);

	if (!feof(stdin)) {
		perror("fread");
		return 1;
	}

	return 0;
}
	