#ifndef SCENE_H
#define SCENE_H

#include "triangle.h"
#include "material.h"

#define MAX_TRIANGLES 100

typedef struct Scene {
    TriangleVertices triangleVertices[MAX_TRIANGLES];
    MaterialHandle triangleMaterialHandles[MAX_TRIANGLES];
    unsigned int triangleCount;
} Scene;

TriangleVertices getTriangleVertices(Scene* scene, TriangleHandle handle) {
    return scene->triangleVertices[handle-1];
}

MaterialHandle getMaterialHandle(Scene* scene, TriangleHandle handle) {
    return scene->triangleMaterialHandles[handle-1];
}

#endif // SCENE_H