
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
