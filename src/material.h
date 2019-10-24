#ifndef MATERIAL_H
#define MATERIAL_H

typedef struct Material {
    float roughness;
    float emission;
    Color3f diffuse;
} Material;

typedef unsigned int MaterialHandle;

#endif // MATERIAL_H