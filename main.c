#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "ppm.h"
#include "vec3.h"
#include "color.h"
#include "triangle.h"
#include "material.h"
#include "scene.h"
#include "util.h"


#define IMAGE_SIZE_X 512
#define IMAGE_SIZE_Y 512
#define MAX_STEPS 32
#define MAX_DISTANCE 100.0f

#define STATS 1

#if STATS
unsigned int totalPrimaryRays = 0;
unsigned int primaryRayHits = 0;
unsigned int totalRays = 0;
unsigned int rayHits = 0;
unsigned int bounceRaysReachedMaxDepth = 0;
unsigned int bounceRaysToLight = 0;
#endif

typedef struct Ray {
    Vec3 origin;
    Norm3 dir;
} Ray;

const Color3f BACKGROUND_COLOR = {0.1f, 0.1f, 0.1f};

const Material materials[] = {
    {.roughness = 1.0f, .emission = 0.0f, .diffuse = { 1.0f, 1.0f, 1.0f}},
    {.roughness = 1.0f, .emission = 0.0f, .diffuse = { 1.0f, 0.05f, 0.05f}},
    {.roughness = 0.0f, .emission = 0.0f, .diffuse = { 0.0f, 1.0f, 0.5f}},
    {.roughness = 1.0f, .emission = 2.0f, .diffuse = { 1.0f, 1.0f, 1.0f}},
};

Material getMaterial(MaterialHandle handle) {
    return materials[handle - 1];
}

Norm3 reflectVec3InHemisphere(Norm3 dirIn, Norm3 normal, float roughness) {
    Vec3 tangent_base = cross(normal, AXIS.up);
    if(squaredLength(tangent_base) < 0.1f) {
        tangent_base = cross(normal, AXIS.forward);
    }
    assert(squaredLength(tangent_base) > 0.01f);
    const Norm3 tangent = normalizeVec3(tangent_base);
    const Norm3 bitangent = cross(normal, tangent);

    const Vec3 rand_vec = {
        .x = ((randFloat() * 2.0f) - 1.0f),
        .y = ((randFloat() * 2.0f) - 1.0f),
        .z = randFloat()
    };
    const Norm3 rand_vec_norm = normalizeVec3(rand_vec);

    const Norm3 dirOut = normalizeVec3(addVec3(
                multVec3Scalar(normal, rand_vec_norm.z), 
        addVec3(multVec3Scalar(tangent, rand_vec_norm.x),
                multVec3Scalar(bitangent, rand_vec_norm.y)
        )
    ));

    return dirOut;
}

typedef struct TriangleIntersection {
    float u, v;
    float distance;
    Vec3 world_pos;
} TriangleIntersection;

typedef struct TriangleHit {
    TriangleIntersection intersection;
    TriangleHandle handle;
    Norm3 normal;
} TriangleHit;

TriangleIntersection intersectTriangle(TriangleVertices vertices, Ray ray) {
    TriangleIntersection result = {
        .distance = MAX_DISTANCE,
        .u = -1.0f,
        .v = -1.0f,
        .world_pos = {0.0f, 0.0f, 0.0f}
    };
    const Vec3 v12 = subVec3(vertices.v2, vertices.v1);
    const Vec3 v13 = subVec3(vertices.v3, vertices.v1);
    const Vec3 plane_normal = cross(v12, v13);
    const Vec3 plane_to_ray = subVec3(ray.origin, vertices.v1);
    const float a = -dot(plane_normal, plane_to_ray);
    const float b = dot(plane_normal, ray.dir);
    const float r = a / b;
    result.distance = r;
    if (r < 0.0f) {
        // ray goes away from triangle
        return result;
    }
    const Vec3 hitPoint = addVec3(ray.origin, multVec3Scalar(ray.dir, r));
    result.world_pos = hitPoint;
    const float uu = dot(v12, v12);
    const float uv = dot(v12, v13);
    const float vv = dot(v13, v13);
    const Vec3 w = subVec3(hitPoint, vertices.v1);
    const float wu = dot(w, v12);
    const float wv = dot(w, v13);
    const float D = uv * uv - uu * vv;

    // get parametric coords
    result.u = (uv * wv - vv * wu) / D;
    result.v = (uv * wu - uu * wv) / D;

    return result;
}

TriangleHit traceRay(Scene* scene, Ray ray) {
    TriangleHit bestHit = {
        .handle = 0,
        .intersection = {
            .distance = MAX_DISTANCE,
            .u = -1.0f,
            .v = -1.0f,
            .world_pos = {0.0f, 0.0f, 0.0f}
        },
        .normal = {0.0f, 0.0f, 1.0f}
    };
    #if STATS
    totalRays++;
    #endif

    for(unsigned int i = 1; i <= scene->triangleCount; i++) {
        TriangleIntersection result = intersectTriangle(getTriangleVertices(scene, i), ray);
        if(result.u >= 0.0f && result.v >= 0.0f && result.u + result.v <= 1.0f) {
            if(result.distance > 0.0f && result.distance < MAX_DISTANCE) {
                bestHit.intersection = result;
                bestHit.handle = i;
            }
        }
    }

    if(bestHit.handle != 0) {
        TriangleVertices vertices = getTriangleVertices(scene, bestHit.handle);
        Vec3 v12 = subVec3(vertices.v2, vertices.v1);
        Vec3 v13 = subVec3(vertices.v3, vertices.v1);
        bestHit.normal = normalizeVec3(cross(v12, v13));
        #if STATS
        rayHits++;
        #endif
    }
    

    return bestHit;
}

Color3f sampleBounceRays(Scene* scene, Ray ray, const unsigned int max_depth) {
    Color3f result = {1.0f, 1.0f, 1.0f};
    //float mix = 1.0f;
    for(unsigned int i = 0; i < max_depth; i++) {
        const TriangleHit hit = traceRay(scene, ray);
        if(hit.handle == 0) {
            return BACKGROUND_COLOR;
        }

        const Material material = getMaterial(getMaterialHandle(scene, hit.handle));
        result = multColor3f(material.diffuse, result);
        //result = mixColor3f(material.diffuse, result, mix);
        //mix *= 0.5f;

        if(material.emission > 0.0f) {
            #if STATS
            bounceRaysToLight++;
            #endif
            result = multColor3fScalar(result, material.emission);
            return result;
        }
        ray.origin = addVec3(hit.intersection.world_pos, multVec3Scalar(hit.normal, 0.001f));
        ray.dir = reflectVec3InHemisphere(ray.dir, hit.normal, material.roughness);
    }
    #if STATS
    bounceRaysReachedMaxDepth++;
    #endif
    return BACKGROUND_COLOR;
}

Color3f samplePixelColor(Scene* scene, const unsigned int x, const unsigned int y, const unsigned int spp) {
    const Vec3 image_plane_point = { 
        .x = (-1.0f + (float)(x) / (float)(IMAGE_SIZE_X) * 2.0f) * 1.5f,
        .y = (1.0f - (float)(y) / (float)(IMAGE_SIZE_Y) * 2.0f)  * 1.5f,
        .z = 1.0f
    };
    const Norm3 dir = normalizeVec3(image_plane_point);
    const Ray ray = {
        .origin = { 0.0f, 0.0f, 0.0f },
        .dir = dir
    };

    const TriangleHit hit = traceRay(scene, ray);
    #if STATS
    totalPrimaryRays++;
    #endif
    
    if(hit.handle != 0) {
        #if STATS
        primaryRayHits++;
        #endif
        const Material primary_hit_material = getMaterial(getMaterialHandle(scene, hit.handle));
        if(primary_hit_material.emission > 0.0f) {
            return multColor3fScalar(primary_hit_material.diffuse, primary_hit_material.emission);
        }
        const Vec3 ray_origin = addVec3(hit.intersection.world_pos, multVec3Scalar(hit.normal, 0.001f));
        Color3f pixel_color_sum = {0.0f, 0.0f, 0.0f};

        for(unsigned int samples = 0; samples < spp; samples++) {
            const Ray ray = {
                .origin = ray_origin,
                .dir = reflectVec3InHemisphere(ray.dir, hit.normal, primary_hit_material.roughness)
            };
            pixel_color_sum = addColor3f(pixel_color_sum, multColor3f(primary_hit_material.diffuse, sampleBounceRays(scene, ray, MAX_STEPS)));
        }
        const float color_factor = 1.0f / (float) spp;
        return multColor3fScalar(pixel_color_sum, color_factor);
    }
    return BACKGROUND_COLOR;
}

int main(int argc, const char** argv) {
    srand((unsigned)time(0));

    Scene scene = {
        .triangleVertices = {
            // Back plane
            {.v1 = {-0.5f, -0.5f, 1.5f}, .v2 = {-0.5f, 0.5f, 1.5f}, .v3 = {0.5f, 0.5f, 1.5f}},
            {.v1 = {-0.5f, -0.5f, 1.5f}, .v2 = {0.5f, 0.5f, 1.5f}, .v3 = {0.5f, -0.5f, 1.5f}},
            // Lighting plane
            {.v1 = {-0.4f, 0.45f, 1.4f}, .v2 = {-0.4f, 0.45f, 0.6f}, .v3 = {0.4f, 0.45f, 0.6f}},
            {.v1 = {-0.4f, 0.45f, 1.4f}, .v2 = {0.4f, 0.45f, 0.6f}, .v3 = {0.4f, 0.45f, 1.4f}},
            // Ground plane
            {.v1 = {-0.5f, -0.5f, 1.5f}, .v2 = {0.5f, -0.5f, 0.5f}, .v3 = {-0.5f, -0.5f, 0.5f}},
            {.v1 = {-0.5f, -0.5f, 1.5f}, .v2 = {0.5f, -0.5f, 1.5f}, .v3 = {0.5f, -0.5f, 0.5f}},
            // Left plane
            {.v1 = {-0.5f, -0.5f, 0.5f}, .v2 = {-0.5f, 0.5f, 0.5f}, .v3 = {-0.5f, 0.5f, 1.5f}},
            {.v1 = {-0.5f, -0.5f, 0.5f}, .v2 = {-0.5f, 0.5f, 1.5f}, .v3 = {-0.5f, -0.5f, 1.5f}},
            // Right plane
            {.v1 = {0.5f, -0.5f, 1.5f}, .v2 = {0.5f, 0.5f, 1.5f}, .v3 = {0.5f, 0.5f, 0.5f}},
            {.v1 = {0.5f, -0.5f, 1.5f}, .v2 = {0.5f, 0.5f, 0.5f}, .v3 = {0.5f, -0.5f, 0.5f}},
            // Top plane
            {.v1 = {-0.5f, 0.5f, 1.5f}, .v2 = {-0.5f, 0.5f, 0.5f}, .v3 = {0.5f, 0.5f, 0.5f}},
            {.v1 = {-0.5f, 0.5f, 1.5f}, .v2 = {0.5f, 0.5f, 0.5f}, .v3 = {0.5f, 0.5f, 1.5f}},
        },
        .triangleMaterialHandles = {
            1,
            1,
            4,
            4,
            1,
            1,
            2,
            2,
            3,
            3,
            1,
            1
        },
        12
    };
    unsigned int spp_input = 32;
    if(argc > 1) {
        spp_input = atoi(argv[1]);
    }
    const unsigned int spp = spp_input;

    const unsigned int IMAGE_DATA_SIZE = IMAGE_SIZE_X * IMAGE_SIZE_Y * 3;
    char data[IMAGE_DATA_SIZE];
    printf("Begin sampling\n");
    clock_t start = clock();
     
    for(unsigned int y = 0; y < IMAGE_SIZE_Y; y++) {
        for(unsigned int x = 0; x < IMAGE_SIZE_X; x++) {
            const unsigned int data_index = (x+y*IMAGE_SIZE_X)*3;
            
            const Color3f pixel_color = samplePixelColor(&scene, x, y, spp);
            data[data_index + 0] = (char)(clamp(pixel_color.r, 0.0f, 1.0f) * 255.0f);
            data[data_index + 1] = (char)(clamp(pixel_color.g, 0.0f, 1.0f) * 255.0f);
            data[data_index + 2] = (char)(clamp(pixel_color.b, 0.0f, 1.0f) * 255.0f);
            if(data_index % (1000000 / spp) * 3 == 0) {
                printf("Finished %.1f%%\n", ((float)(data_index) / (float)(IMAGE_DATA_SIZE)) * 100.0f);
            }
        }
    }
    clock_t end = clock();
    double cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("Finished sampling\n", cpu_time_used);

    write_ppm("test.ppm", IMAGE_SIZE_X, IMAGE_SIZE_Y, data);
    const unsigned int totalBounceRays = totalRays - totalPrimaryRays;
    const unsigned int bounceRayHits = rayHits - primaryRayHits;

    #if STATS
    printf("Statistics\n");
    printf("==========\n");
    printf("Total time:                            %16.3fs\n", cpu_time_used);
    printf("Total rays:                            %16d    %.0f rays/s\n", totalRays, (double)(totalRays) / cpu_time_used);
    printf("Total primary rays:                    %16d\n", totalPrimaryRays);
    printf("Total bounce rays:                     %16d\n", totalBounceRays);
    printf("---\n");
    printf("Primary ray hits:                      %16d    %5.2f%%\n", primaryRayHits, ((double)(primaryRayHits) / (double)(totalPrimaryRays))*100.0);
    printf("Bounce ray hits:                       %16d    %5.2f%%\n", bounceRayHits, ((double)(bounceRayHits) / (double)(totalBounceRays))*100.0);
    printf("Bounce rays to light source:           %16d    %5.2f%%\n", bounceRaysToLight, ((double)(bounceRaysToLight) / (double)(primaryRayHits * spp))*100.0);
    printf("Bounce rays with max depth:            %16d    %5.2f%%\n", bounceRaysReachedMaxDepth, ((double)(bounceRaysReachedMaxDepth) / (double)(primaryRayHits * spp))*100.0);
    printf("---\n");
    printf("Avg bounce ray depth:                  %16.2f\n", ((double)(totalBounceRays)/ (double)(spp)) / ((double)(primaryRayHits)));
    #endif
    
    return 0;
}