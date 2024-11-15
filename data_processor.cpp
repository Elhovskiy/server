#include "data_processor.h"

#define OVERFLOW_VALUE -32767

int16_t processVector(const std::vector<int16_t>& vector) {
    int64_t sum = 0;
    for (int16_t value : vector) {
        sum += value;
        if (sum > INT16_MAX) return OVERFLOW_VALUE;
        if (sum < INT16_MIN) return OVERFLOW_VALUE;
    }
    return static_cast<int16_t>(sum);
}
