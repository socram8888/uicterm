
#include "telegram.h"
#include <stdint.h>

struct telegram {
	telegram_status_t status;

	int bit_count;
	uint_least64_t bits;
	uint8_t correct_crc;
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

bool telegram_is_done(telegram_t * t) {
	return t->status == TELEGRAM_OK || t->status == TELEGRAM_INTEGRITY;
}

int telegram_train_number(telegram_t * t) {
	if (!telegram_is_done(t)) {
		return -1;
	}

	uint32_t train = (t->bits >> 15) & 0xFFFFFF;

	// The bits for each BCD digit are in reverse - undo it
	train = (train & 0xAAAAAA) >> 1 | (train & 0x555555) << 1;
	train = (train & 0xCCCCCC) >> 2 | (train & 0x333333) << 2;

	return train;
}

int telegram_code_number(telegram_t * t) {
	if (!telegram_is_done(t)) {
		return -1;
	}

	return (t->bits >> 7) & 0xFF;
}

int telegram_received_crc(telegram_t * t) {
	if (!telegram_is_done(t)) {
		return -1;
	}

	// Bits are inverted
	return (t->bits & 0x7F) ^ 0x7F;
}

int telegram_correct_crc(telegram_t * t) {
	return t->correct_crc;
}

int64_t telegram_raw(telegram_t * t) {
	if (!telegram_is_done(t)) {
		return -1;
	}

	return t->bits & 0x7FFFFFFFFFLL;
}

#define CRC_POLY 0xE1
uint_least64_t crc_calculate(uint_least64_t bits) {
	for (int bpos = 38; bpos >= 7; bpos--) {
		if (bits & (1ULL << bpos)) {
			bits ^= (uint64_t) CRC_POLY << (bpos - 7);
		}
	}

	return bits;
}

void telegram_feed(telegram_t * t, int bit) {
	if (telegram_is_done(t)) {
		t->bit_count = 0;
		t->status = TELEGRAM_MORE;
	}

	t->bits = t->bits << 1 | bit;
	if (t->bit_count < 51) {
		t->bit_count++;

		if (t->bit_count < 51) {
			return;
		}
	}

	uint32_t syncbits = (t->bits >> 39) & 0xFFF;
	if (syncbits != 0xFF2) {
		t->status = TELEGRAM_NO_SYNC;
		return;
	}

	uint8_t received_crc = (t->bits & 0x7F) ^ 0x7F;
	t->correct_crc = crc_calculate(t->bits & ~0x7F) & 0x7F;

	if (received_crc == t->correct_crc) {
		t->status = TELEGRAM_OK;
	} else {
		t->status = TELEGRAM_INTEGRITY;
	}
}

void telegram_reset(telegram_t * t) {
	t->status = TELEGRAM_MORE;
	t->bit_count = 0;
}

void telegram_free(telegram_t * t) {
	free(t);
}
