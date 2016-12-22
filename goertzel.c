
#include "goertzel.h"
#include <stdlib.h>
#include <math.h>

struct goertzel {
	size_t freq_count;
	float * coeffs;
};

// M_PI isn't really standard - define here our own version
#define PI 3.14159265358979323846264338327950288419716939937510582

goertzel_t * goertzel_init(const float * frequencies, size_t freq_count, float sample_rate) {
	/********************
	 * malloc structure *
	 ********************/
	goertzel_t * g = malloc(sizeof(struct goertzel));
	if (g == NULL) {
		return NULL;
	}

	g->freq_count = freq_count;

	/**************************
	 * calculate coefficients *
	 **************************/
	g->coeffs = malloc(sizeof(float) * freq_count);
	if (g->coeffs == NULL) {
		goertzel_free(g);
		return NULL;
	}

	for (size_t i = 0; i < freq_count; i++) {
		g->coeffs[i] = 2 * cos(2 * PI * frequencies[i] / sample_rate);
	}
	
	return g;
}

void goertzel_magnitude(goertzel_t * g, const float * samples, size_t sample_count, float * magnitude) {
	float * coeffs = g->coeffs;

	for (size_t freq = 0; freq < g->freq_count; freq++) {
		float reallyold, old = 0, current = 0;

		for (size_t sample = 0; sample < sample_count; sample++) {
			reallyold = old;
			old = current;
			current = samples[sample] + coeffs[freq] * old - reallyold;
		}

		magnitude[freq] = sqrt(current * current + old * old - current * old * coeffs[freq]);
	}
}

void goertzel_free(goertzel_t * g) {
	if (g == NULL) {
		return;
	}

	free(g->coeffs);
	free(g);
}
