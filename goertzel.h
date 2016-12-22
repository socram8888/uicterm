
#pragma once
#include <stdlib.h>

typedef struct goertzel goertzel_t;

/**
 * Initializes a new Goertzel filter, precalculating the coefficients for the given frequencies.
 *
 * @param frequencies Frequency array
 * @param freq_count Number of frequencies in array
 * @param sample_rate Input sample rate
 * @returns New Goertzel filter, or NULL on error
 */
goertzel_t * goertzel_init(const float * frequencies, size_t freq_count, float sample_rate);

/**
 * Calculates the relative Goertzel magnitude for given samples.
 *
 * @param g Goertzel filter
 * @param samples Input samples
 * @param sample_count Number of samples in input
 * @param magnitude Calculated relative magnitudes, not squared
 */
void goertzel_magnitude(goertzel_t * g, const float * samples, size_t sample_count, float * magnitude);

/**
 * Destroys a Goertzel filter.
 *
 * @param g Goertzel filter
 */
void goertzel_free(goertzel_t * g);
