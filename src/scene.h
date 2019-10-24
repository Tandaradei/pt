#ifndef SCENE_H
#define SCENE_H

#include "triangle.h"
#include "material.h"

#define MAX_TRIANGLES 100

typedef struct Scene {
    TriangleVertices triangle_vertices[MAX_TRIANGLES];
    MaterialHandle triangle_material_handles[MAX_TRIANGLES];
    unsigned int triangle_count;
} Scene;

TriangleVertices getTriangleVertices(Scene* scene, TriangleHandle handle) {
    return scene->triangle_vertices[handle-1];
}

MaterialHandle getMaterialHandle(Scene* scene, TriangleHandle handle) {
    return scene->triangle_material_handles[handle-1];
}

#endif // SCENE_H