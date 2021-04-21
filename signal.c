
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
