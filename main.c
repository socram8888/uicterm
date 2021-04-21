
#include <assert.h>
#include <pulse/error.h>
#include <pulse/simple.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <math.h>

#include "uicdemod.h"
#include "telegram.h"
#include "signal.h"

#define DEFAULT_SAMPLE_RATE 16000
#define DEFAULT_BUFFER_MILLIS 50
#define DEFAULT_TICKS 2
#define DEFAULT_CERTAINTY 0.75

// From http://stackoverflow.com/a/7924459
#ifdef _WIN32
#	include <fcntl.h>
#	include <io.h>
#endif

static const char * me;

struct context {
	const char * source_name;
	int sample_rate;

	pa_simple * pulse_source;
	int16_t * int_buffer;
	float * float_buffer;
	size_t sample_count;
	float tone_certainty;
	int required_ticks;
	bool show_raw_telegrams;
	bool hide_damaged;

	uicdemod_t * uic;
};

void show_usage() {
	fprintf(stderr,
			"UIC-751-3 demodulator\n"
			"Usage: %s [OPTION]\n"
			"Reads audio from standard input and decodes digital information "
			"information according to railway standard UIC-751-3\n"
			"\n"
			"Audio options:\n"
			"  -s[SOURCE]  pulse audio source name (required)\n"
			"  -r[RATE]    sets input sample rate (default: %d)\n"
			"  -b[MILLIS]  sets input buffer length, in milliseconds (default: %dms)\n"
			"  -c[TH]      normalized threshold for a tone to be detected as present (default: %f)\n"
			"  -t[TICKS]   number of consecutive buffers to have a tone before printing it (default: %d)\n"
			"  -u          show unparsed, raw telegram bits\n"
			"  -d          hide damaged packets not passing integrity checks\n"
			"\n"
			"Miscellaneous options:\n"
			"  -h, -?      shows this help text\n",
			me, DEFAULT_SAMPLE_RATE, DEFAULT_BUFFER_MILLIS, DEFAULT_CERTAINTY, DEFAULT_TICKS
	);
}

bool parse_config(struct context * ctx, int argc, char ** argv) {
	me = argv[0];

	ctx->sample_rate = DEFAULT_SAMPLE_RATE;
	ctx->required_ticks = DEFAULT_TICKS;
	ctx->tone_certainty = DEFAULT_CERTAINTY;

	int buffer_millis = DEFAULT_BUFFER_MILLIS;

	int c;
	while ((c = getopt(argc, argv, "hs:r:b:t:c:ud")) != -1) {
		switch (c) {
			case 'h':
			case '?':
				show_usage();
				return false;

			case 's':
				ctx->source_name = optarg;
				break;

			case 'r':
				ctx->sample_rate = atoi(optarg);
				break;

			case 'b':
				buffer_millis = atoi(optarg);
				break;

			case 'c':
				ctx->tone_certainty = atof(optarg);
				break;

			case 't':
				ctx->required_ticks = atoi(optarg);
				break;

			case 'u':
				ctx->show_raw_telegrams = true;
				break;

			case 'd':
				ctx->hide_damaged = true;
				break;

			default:
				fprintf(stderr, "Error: unknown option \"%c\"", c);
				return false;
		}
	}

	if (!ctx->source_name) {
		fprintf(stderr, "Error: PulseAudio source not set\n");
		return false;
	}

	if (ctx->sample_rate <= 0) {
		fprintf(stderr, "Error: invalid sample rate\n");
		return false;
	} else if (ctx->sample_rate < 11800) {
		fprintf(stderr, "Warning: sample rate is too low, consider using 11800Hz or more\n");
	}

	if (buffer_millis <= 0) {
		fprintf(stderr, "Error: invalid buffer length\n");
		return false;
	}

	if (ctx->tone_certainty < 0 || ctx->tone_certainty > 1) {
		fprintf(stderr, "Error: tone should be between 0 and 1\n");
		return false;
	}

	if (ctx->required_ticks < 1) {
		fprintf(stderr, "Error: required signal ticks must be at least one\n");
		return false;
	}

	ctx->sample_count = ceil(buffer_millis * ctx->sample_rate / 1000);

	return true;
}

void destroy_ctx(struct context * ctx) {
	free(ctx->int_buffer);
	free(ctx->float_buffer);
	uicdemod_free(ctx->uic);
}

bool init_ctx(struct context * ctx) {
	#ifdef _WIN32
		_setmode(_fileno(stdin), _O_BINARY);
	#endif

	int pa_error;
	pa_sample_spec pa_spec = {
		.format = PA_SAMPLE_S16LE,
		.rate = ctx->sample_rate,
		.channels = 1
	};
	ctx->pulse_source = pa_simple_new(NULL, me, PA_STREAM_RECORD, ctx->source_name, "uicterm", &pa_spec, NULL, NULL, &pa_error);
	if (!ctx->pulse_source) {
		fprintf(stderr, "Error: pa_simple_new() failed: %s\n", pa_strerror(pa_error));
		destroy_ctx(ctx);
		return false;
	}

	ctx->int_buffer = malloc(ctx->sample_count * sizeof(int16_t));
	if (ctx->int_buffer == NULL) {
		fprintf(stderr, "Error: could not allocate buffer for %u integers\n", (unsigned int) ctx->sample_count);
		destroy_ctx(ctx);
		return false;
	}

	ctx->float_buffer = malloc(ctx->sample_count * sizeof(float));
	if (ctx->float_buffer == NULL) {
		fprintf(stderr, "Error: could not allocate buffer for %u floats\n", (unsigned int) ctx->sample_count);
		destroy_ctx(ctx);
		return false;
	}

	ctx->uic = uicdemod_init(ctx->sample_rate);
	if (ctx->uic == NULL) {
		fprintf(stderr, "Error: could not initialize UIC demodulator\n");
		destroy_ctx(ctx);
		return false;
	}

	uicdemod_set_tone_certainty(ctx->uic, ctx->tone_certainty);
	uicdemod_set_required_ticks(ctx->uic, ctx->required_ticks);

	return true;
}

void print_bits(uint64_t bits, int_least8_t len) {
	uint64_t mask = (1ULL << len) >> 1;

	while (mask != 0) {
		if (bits & mask) {
			printf("1");
		} else {
			printf("0");
		}
		mask = mask >> 1;
	}
}

void print_event(struct context * ctx, uicdemod_status_t event) {
	telegram_t * telegram;
	switch (event) {
		case UICDEMOD_PACKET:
			telegram = uicdemod_get_telegram(ctx->uic);

			switch (telegram_status(telegram)) {
				case TELEGRAM_OK:
					printf(
							"Packet %06X %02X\n",
							telegram_train_number(telegram),
							telegram_code_number(telegram)
					);
					break;

				case TELEGRAM_INTEGRITY:
					if (!ctx->hide_damaged) {
						printf(
							"Packet %06X %02X (received CRC: %02X, correct: %02X)\n",
							telegram_train_number(telegram),
							telegram_code_number(telegram),
							telegram_received_crc(telegram),
							telegram_correct_crc(telegram)
						);
					}
					break;

				default:
					// Should never happen
					assert(0);
			}

			if (ctx->show_raw_telegrams) {
				printf("Raw packet: ");
				print_bits(telegram_raw(telegram), 39);
				printf("\n");
			}

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
		default:
			// Should never happen
			assert(0);
			break;
	}
	fflush(stdout);
}

bool read_loop(struct context * ctx) {
	while (1) {
		int pa_error;
		if (pa_simple_read(ctx->pulse_source, ctx->int_buffer, ctx->sample_count * sizeof(int16_t), &pa_error) < 0) {
			fprintf(stderr, "Error: pa_simple_read() failed: %s\n", pa_strerror(pa_error));
			return false;
		}

		int16_to_float(ctx->int_buffer, ctx->float_buffer, ctx->sample_count);

		uicdemod_analyze_begin(ctx->uic);

		const float * sample_ptr = ctx->float_buffer;
		size_t remaining_samples = ctx->sample_count;
		uicdemod_status_t event = uicdemod_analyze(ctx->uic, &sample_ptr, &remaining_samples);
		while (event != UICDEMOD_NONE) {
			print_event(ctx, event);
			event = uicdemod_analyze(ctx->uic, &sample_ptr, &remaining_samples);
		}
	}
}

int main(int argc, char ** argv) {
	struct context ctx = { 0 };

	if (!parse_config(&ctx, argc, argv)) {
		return 1;
	}

	if (!init_ctx(&ctx)) {
		return 2;
	}

	if (!read_loop(&ctx)) {
		destroy_ctx(&ctx);
		return 3;
	}

	destroy_ctx(&ctx);
	return 0;
}
