
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <math.h>

#include "uicdemod.h"
#include "telegram.h"
#include "signal.h"

#define DEFAULT_SAMPLE_RATE 44100.0
#define DEFAULT_BUFFER_MILLIS 50.0
#define DEFAULT_TICKS 2
#define DEFAULT_CERTAINTY 0.75

// From http://stackoverflow.com/a/7924459
#ifdef _WIN32
#	include <fcntl.h>
#	include <io.h>
#endif

static const char * me;

struct context {
	float sample_rate;

	int16_t * int_buffer;
	float * float_buffer;
	size_t sample_count;
	float tone_certainty;
	int required_ticks;
	bool show_raw_telegrams;

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
			"  -r[RATE]    sets input sample rate (default: %f)\n"
			"  -b[MILLIS]  sets input buffer length, in milliseconds (default: %fms)\n"
			"  -c[TH]      normalized threshold for a tone to be detected as present (default: %f)\n"
			"  -t[TICKS]   number of consecutive buffers to have a tone before printing it (default: %d)\n"
			"  -u          show unparsed, raw telegram bits\n"
			"\n"
			"Miscellaneous options:\n"
			"  -h, -?      shows this help text\n",
			me, DEFAULT_SAMPLE_RATE, DEFAULT_BUFFER_MILLIS, DEFAULT_CERTAINTY, DEFAULT_TICKS
	);
}

bool parse_config(struct context * ctx, int argc, char ** argv) {
	me = argv[0];

	float sample_rate = DEFAULT_SAMPLE_RATE;
	float buffer_millis = DEFAULT_BUFFER_MILLIS;
	int required_ticks = DEFAULT_TICKS;
	float tone_certainty = DEFAULT_CERTAINTY;

	int c;
	ctx->show_raw_telegrams = false;
	while ((c = getopt(argc, argv, "hr:b:t:c:u")) != -1) {
		switch (c) {
			case 'h':
			case '?':
				show_usage();
				return false;

			case 'r':
				sample_rate = atof(optarg);
				break;

			case 'b':
				buffer_millis = atof(optarg);
				break;

			case 'c':
				tone_certainty = atof(optarg);
				break;

			case 't':
				required_ticks = atoi(optarg);
				break;

			case 'u':
				ctx->show_raw_telegrams = true;
				break;

			default:
				fprintf(stderr, "Error: unknown option \"%c\"", c);
				return false;
		}
	}

	if (sample_rate <= 0) {
		fprintf(stderr, "Error: invalid sample rate\n");
		return false;
	} else if (sample_rate < 11800) {
		fprintf(stderr, "Warning: sample rate is too low, consider using 11800Hz or more\n");
	}

	if (buffer_millis <= 0) {
		fprintf(stderr, "Error: invalid buffer length\n");
		return false;
	}

	if (tone_certainty < 0 || tone_certainty > 1) {
		fprintf(stderr, "Error: tone should be between 0 and 1\n");
		return false;
	}

	if (required_ticks < 1) {
		fprintf(stderr, "Error: required signal ticks must be at least one\n");
		return false;
	}

	ctx->sample_rate = sample_rate;
	ctx->sample_count = ceil(buffer_millis * sample_rate / 1000);
	ctx->tone_certainty = tone_certainty;
	ctx->required_ticks = required_ticks;

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
			printf("Packet %06X %02X (CRC: %02X)\n", telegram_train_number(telegram), telegram_code_number(telegram), telegram_crc(telegram));
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
		size_t sample_count = fread(ctx->int_buffer, sizeof(int16_t), ctx->sample_count, stdin);
		if (sample_count == 0) {
			return feof(stdin);
		}

		int16_to_float(ctx->int_buffer, ctx->float_buffer, sample_count);

		uicdemod_analyze_begin(ctx->uic);

		const float * sample_ptr = ctx->float_buffer;
		uicdemod_status_t event = uicdemod_analyze(ctx->uic, &sample_ptr, &sample_count);
		while (event != UICDEMOD_NONE) {
			print_event(ctx, event);
			event = uicdemod_analyze(ctx->uic, &sample_ptr, &sample_count);
		}
	}
}

int main(int argc, char ** argv) {
	struct context ctx;
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
