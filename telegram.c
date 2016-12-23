
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

	// TODO: figure bit mask
	return 0;
}

int telegram_code_number(telegram_t * t) {
	if (t->status != TELEGRAM_OK) {
		return -1;
	}

	// TODO: figure bit mask
	return 0;
}

telegram_status_t telegram_feed(telegram_t * t, int bit) {
	if (t->status == TELEGRAM_OK) {
		t->bit_count = 0;
	}

	// TODO: do stuff

	return t->status;
}

void telegram_reset(telegram_t * t) {
	t->status = TELEGRAM_MORE;
	t->bit_count = 0;
}

void telegram_free(telegram_t * t) {
	free(t);
}
