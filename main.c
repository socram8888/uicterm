
#include <stdio.h>
#include <stdint.h>
#include "uicdemod.h"
#include "telegram.h"
#include "signal.h"

#define SAMPLE_RATE 8000
#define SAMPLE_COUNT SAMPLE_RATE / 20

// From http://stackoverflow.com/a/7924459
#ifdef _WIN32
#	include <fcntl.h>
#	include <io.h>
#endif

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

	size_t sample_count = fread(sample_ints, sizeof(int16_t), SAMPLE_COUNT, stdin);
	while (sample_count > 0) {
		float samples[SAMPLE_COUNT];
		int16_to_float(sample_ints, samples, sample_count);

		uicdemod_analyze_begin(uic);

		const float * sample_ptr = samples;
		uicdemod_status_t event = uicdemod_analyze(uic, &sample_ptr, &sample_count);
		while (event != UICDEMOD_NONE) {
			telegram_t * telegram;
			switch (event) {
				case UICDEMOD_NONE:
					assert(0);
					break;
				case UICDEMOD_PACKET:
					telegram = uicdemod_get_telegram(uic);
					printf("Packet %06X %02X\n", telegram_train_number(telegram), telegram_code_number(telegram));
					break;
				case UICDEMOD_WARNING:
					printf("Warning\n");
					break;
				case UICDEMOD_LISTENING:
					printf("Listening\n");
					break;
				case UICDEMOD_CHFREE:
					printf("Channel free\n");
					break;
				case UICDEMOD_PILOT:
					printf("Voice pilot\n");
					break;
				case UICDEMOD_SILENCE:
					printf("Silence\n");
					break;
			}
			fflush(stdout);

			event = uicdemod_analyze(uic, &sample_ptr, &sample_count);
		}

		sample_count = fread(sample_ints, sizeof(int16_t), SAMPLE_COUNT, stdin);
	}

	uicdemod_free(uic);

	if (!feof(stdin)) {
		perror("fread");
		return 1;
	}

	return 0;
}
	