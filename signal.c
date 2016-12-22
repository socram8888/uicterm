
#include "signal.h"

void normalize(const float * input, float * output, size_t count) {
	float sum = 0;

	for (size_t i = 0; i < count; i++) {
		sum += input[i];
	}

	for (size_t i = 0; i < count; i++) {
		output[i] = input[i] / sum;
	}
}

void int16_to_float(const int16_t * input, float * output, size_t count) {
	for (size_t i = 0; i < count; i++) {
		// 16-bit integers have a range biased towards negative - add .5 to nudge the center
		output[i] = input[i] / 32768.0 + 0.5;
	}
}
