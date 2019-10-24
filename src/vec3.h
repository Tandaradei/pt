#ifndef VEC_3_H
#define VEC_3_H

typedef union Vec3 {
    struct {
        float x, y, z;
    };
    struct {
        float v[3];
    };
} Vec3;

typedef union Axis {
    struct {
        Vec3 left;
        Vec3 right;
        Vec3 down;
        Vec3 up;
        Vec3 back;
        Vec3 forward;
    };
    struct {
        Vec3 axis[6];
    };
} Axis;

extern const Axis AXIS;

typedef Vec3 Norm3;

float squaredLength(Vec3 vec);

float length(Vec3 vec);

Norm3 normalizeVec3(Vec3 vec);

Vec3 addVec3(Vec3 a, Vec3 b);

Vec3 subVec3(Vec3 a, Vec3 b);

Vec3 multVec3Scalar(Vec3 vec, float scalar);

Vec3 cross(Vec3 a, Vec3 b);

float dot(Vec3 a, Vec3 b);

#endif // VEC_3_H