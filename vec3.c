#include "vec3.h"

#include <assert.h>
#include <math.h>

const Axis AXIS = {
    .left    = {-1.0f,  0.0f,  0.0f},
    .right   = { 1.0f,  1.0f,  0.0f},
    .down    = { 0.0f, -1.0f,  0.0f},
    .up      = { 0.0f,  1.0f,  0.0f},
    .back    = { 0.0f,  0.0f, -1.0f},
    .forward = { 0.0f,  0.0f,  1.0f},
};

float squaredLength(Vec3 vec) {
    return vec.x * vec.x + vec.y * vec.y + vec.z * vec.z;
}

float length(Vec3 vec) {
    return sqrt(squaredLength(vec));
}

Norm3 normalizeVec3(Vec3 vec) {
    assert(vec.x != 0.0f || vec.y != 0.0f || vec.z != 0.0f);
    float len = length(vec);
    Norm3 result = {
        .x = vec.x / len,
        .y = vec.y / len,
        .z = vec.z / len
    };
    return result;
}

Vec3 addVec3(Vec3 a, Vec3 b) {
    Vec3 result = {
        a.x + b.x,
        a.y + b.y,
        a.z + b.z
    };
    return result;
}

Vec3 subVec3(Vec3 a, Vec3 b) {
    Vec3 result = {
        a.x - b.x,
        a.y - b.y,
        a.z - b.z
    };
    return result;
}

Vec3 multVec3Scalar(Vec3 vec, float scalar) {
    Vec3 result = {
        vec.x * scalar,
        vec.y * scalar,
        vec.z * scalar,
    };
    return result;
}

Vec3 cross(Vec3 a, Vec3 b) {
    Vec3 result = {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
    return result;
}

float dot(Vec3 a, Vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}