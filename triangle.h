#ifndef TRIANGLE_H
#define TRIANGLE_H

#include "vec3.h"

typedef union TriangleVertices {
    struct {
        Vec3 v1, v2, v3;
    };
    struct {
        Vec3 v[3];
    };
} TriangleVertices;

typedef unsigned int TriangleHandle;

#endif // TRIANGLE_H