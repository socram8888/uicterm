
#include "telegram.h"
#include <stdint.h>

struct telegram {
	telegram_status_t status;

	int bit_count;
	uint_least64_t bits;
};

telegram_t * telegram_init() {
	telegram_t * t = malloc(sizeof(struct telegram));
	if (t == NULL) {
		return NULL;
	}

	t->status = TELEGRAM_MORE;
	t->bit_count = 0;

	return t;
}

telegram_status_t telegram_status(telegram_t * t) {
	return t->status;
}

int telegram_train_number(telegram_t * t) {
	if (t->status != TELEGRAM_OK) {
		return -1;
	}

	uint32_t train = (t->bits >> 15) & 0xFFFFFF;

	// The bits for each BCD digit are in reverse - undo it
	train = (train & 0xAAAAAA) >> 1 | (train & 0x555555) << 1;
	train = (train & 0xCCCCCC) >> 2 | (train & 0x333333) << 2;

	return train;
}

int telegram_code_number(telegram_t * t) {
	if (t->status != TELEGRAM_OK) {
		return -1;
	}

	return (t->bits >> 7) & 0xFF;
}

telegram_status_t telegram_feed(telegram_t * t, int bit) {
	if (t->status == TELEGRAM_OK) {
		t->bit_count = 0;
		t->status = TELEGRAM_MORE;
	}

	t->bits = t->bits << 1 | bit;
	if (t->bit_count < 51) {
		t->bit_count++;

		if (t->bit_count < 51) {
			return t->status;
		}
	}

	uint32_t syncbits = (t->bits >> 39) & 0xFFF;
	if (syncbits != 0xFF2) {
		t->status = TELEGRAM_NO_SYNC;
		return t->status;
	}

	// TODO: how does the CRC work? figure out and check

	t->status = TELEGRAM_OK;
	return t->status;
}

void telegram_reset(telegram_t * t) {
	t->status = TELEGRAM_MORE;
	t->bit_count = 0;
}

void telegram_free(telegram_t * t) {
	free(t);
}
