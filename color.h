#ifndef COLOR_H
#define COLOR_H

typedef union Color3f {
    struct {
        float r, g, b;
    };
    struct {
        float v[3];
    };
} Color3f;


Color3f mixColor3f(Color3f a, Color3f b, float mix) {
    Color3f result = {
        .r = a.r * mix + b.r * (1.0f - mix),
        .g = a.g * mix + b.g * (1.0f - mix),
        .b = a.b * mix + b.b * (1.0f - mix)
    };
    return result;
}

Color3f addColor3f(Color3f a, Color3f b) {
    Color3f result = {
        .r = a.r + b.r,
        .g = a.g + b.g,
        .b = a.b + b.b
    };
    return result;
}

Color3f multColor3f(Color3f a, Color3f b) {
    Color3f result = {
        .r = a.r * b.r,
        .g = a.g * b.g,
        .b = a.b * b.b
    };
    return result;
}

Color3f multColor3fScalar(Color3f a, float scalar) {
    Color3f result = {
        .r = a.r * scalar,
        .g = a.g * scalar,
        .b = a.b * scalar
    };
    return result;
}

#endif // COLOR_H