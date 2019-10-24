#ifndef UTIL_H
#define UTIL_H

#include <stdlib.h>

inline float clamp(float value, float min, float max) {
    return value < min ? min : (value > max ? max : value); 
}

inline float randFloat() {
    return (float)rand()/(float)(RAND_MAX);
}

#endif // UTIL_H