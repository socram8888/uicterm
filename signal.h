
#include <stdint.h>
#include <stdlib.h>

/**
 * Normalizes the samples, scaling them so they sum one.
 *
 * @param input Input samples
 * @param output Scaled samples
 * @param count Sample count
 */
void normalize(const float * input, float * output, size_t count);

/**
 * Converts a signed 16 bit integer array to floats in range [-1, 1]
 *
 * @param input Input samples
 * @param output Float array
 * @param count Float count
 */
void int16_to_float(const int16_t * input, float * output, size_t count);
