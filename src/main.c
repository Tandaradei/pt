#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <omp.h>

#include "ppm.h"
#include "vec3.h"
#include "color.h"
#include "triangle.h"
#include "material.h"
#include "scene.h"
#include "util.h"
#define STATS 1
#include "stats.h"

Stats global_stats = {};

#define IMAGE_SIZE_X 512
#define IMAGE_SIZE_Y 512
#define MAX_STEPS 16
#define MAX_DISTANCE 100.0f

#define PLANE(a, b, c, d) (TriangleVertices){a, b, c}, (TriangleVertices){c, d, a}

typedef struct Ray {
    Vec3 origin;
    Norm3 dir;
} Ray;

const Color3f BACKGROUND_COLOR = {1.0f, 1.0f, 1.0f};

const Material materials[] = {
    {.roughness = 1.0f, .emission = 0.0f, .diffuse = { 1.0f, 1.0f, 1.0f}},
    {.roughness = 1.0f, .emission = 0.0f, .diffuse = { 1.0f, 0.05f, 0.05f}},
    {.roughness = 0.0f, .emission = 0.0f, .diffuse = { 0.0f, 1.0f, 0.5f}},
    {.roughness = 1.0f, .emission = 2.0f, .diffuse = { 1.0f, 0.05f, 0.01f}},
    {.roughness = 1.0f, .emission = 4.0f, .diffuse = { 0.1f, 0.02f, 1.0f}},
    {.roughness = 1.0f, .emission = 0.0f, .diffuse = { 0.05f, 0.05f, 1.0f}},
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

TriangleHit traceRay(Scene* scene, const Ray ray) {
    TriangleHit best_hit = {
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
    global_stats.ray_count++;
    #endif

    for(unsigned int i = 1; i <= scene->triangle_count; i++) {
        TriangleIntersection result = intersectTriangle(getTriangleVertices(scene, i), ray);
        if(result.u >= 0.0f && result.v >= 0.0f && result.u + result.v <= 1.0f) {
            if(result.distance > 0.0f && result.distance < best_hit.intersection.distance) {
                best_hit.intersection = result;
                best_hit.handle = i;
            }
        }
    }

    if(best_hit.handle != 0) {
        TriangleVertices vertices = getTriangleVertices(scene, best_hit.handle);
        Vec3 v12 = subVec3(vertices.v2, vertices.v1);
        Vec3 v13 = subVec3(vertices.v3, vertices.v1);
        best_hit.normal = normalizeVec3(cross(v12, v13));
        #if STATS
        global_stats.ray_hits++;
        #endif
    }

    return best_hit;
}

Color3f sampleBounceRay(Scene* scene, Ray ray, const unsigned int max_depth) {
    Color3f result = {1.0f, 1.0f, 1.0f};
    for(unsigned int i = 0; i < max_depth; i++) {
        const TriangleHit hit = traceRay(scene, ray);
        if(hit.handle == 0) {
            return multColor3f(BACKGROUND_COLOR, result);;
        }

        const Material material = getMaterial(getMaterialHandle(scene, hit.handle));
        result = multColor3f(material.diffuse, result);

        if(material.emission > 0.0f) {
            #if STATS
            global_stats.bounce_rays.hit_emissive++;
            #endif
            return multColor3fScalar(result, material.emission);
        }
        ray.origin = addVec3(hit.intersection.world_pos, multVec3Scalar(hit.normal, 0.001f));
        ray.dir = reflectVec3InHemisphere(ray.dir, hit.normal, material.roughness);
    }
    #if STATS
    global_stats.bounce_rays.reached_max_depth++;
    #endif
    return (Color3f){0.0f, 0.0f, 0.0f};
}

Color3f samplePixelColor(Scene* scene, const unsigned int x, const unsigned int y, const unsigned int spp) {
    const Norm3 origin_to_image_plane_point = normalizeVec3((Vec3){ 
        .x = (-1.0f + (float)(x) / (float)(IMAGE_SIZE_X) * 2.0f),
        .y = (1.0f - (float)(y) / (float)(IMAGE_SIZE_Y) * 2.0f),
        .z = 1.0f
    });
    const Ray primary_ray = {
        .origin = { 0.0f, 0.0f, -0.5f },
        .dir = origin_to_image_plane_point
    };

    const TriangleHit primary_hit = traceRay(scene, primary_ray);
    #if STATS
    global_stats.primary_rays.count++;
    #endif
    
    if(primary_hit.handle != 0) {
        #if STATS
        global_stats.primary_rays.hits++;
        #endif
        const MaterialHandle material_handle = getMaterialHandle(scene, primary_hit.handle);
        const Material primary_hit_material = getMaterial(material_handle);
        if(primary_hit_material.emission > 0.0f) {
            #if STATS
            global_stats.primary_rays.hit_emissive++;
            #endif
            return multColor3fScalar(primary_hit_material.diffuse, primary_hit_material.emission);
        }
        const Vec3 ray_origin = addVec3(primary_hit.intersection.world_pos, multVec3Scalar(primary_hit.normal, 0.001f));
        Color3f pixel_color_sum = {0.0f, 0.0f, 0.0f};

        for(unsigned int samples = 0; samples < spp; samples++) {
            const Ray ray = {
                .origin = ray_origin,
                .dir = reflectVec3InHemisphere(primary_ray.dir, primary_hit.normal, primary_hit_material.roughness)
            };
            const Color3f bounce_ray_color = sampleBounceRay(scene, ray, MAX_STEPS);
            pixel_color_sum = addColor3f(pixel_color_sum, multColor3f(primary_hit_material.diffuse, bounce_ray_color));
        }
        const float color_factor = 1.0f / (float) spp;
        return multColor3fScalar(pixel_color_sum, color_factor);
    }
    return BACKGROUND_COLOR;
}

int main(int argc, const char** argv) {
    srand((unsigned)time(NULL));

    const Vec3 box[8] = {
        {{-0.5f, -0.5f, 0.0f}}, // LBF 0
        {{-0.5f, -0.5f, 1.0f}}, // LBB 1
        {{-0.5f,  0.5f, 0.0f}}, // LTF 2
        {{-0.5f,  0.5f, 1.0f}}, // LTB 3
        {{ 0.5f, -0.5f, 0.0f}}, // RBF 4
        {{ 0.5f, -0.5f, 1.0f}}, // RBB 5
        {{ 0.5f,  0.5f, 0.0f}}, // RTF 6
        {{ 0.5f,  0.5f, 1.0f}}, // RTB 7
    };

    Scene scene = {
        .triangle_vertices = {
            // Back plane
            PLANE(box[1], box[3], box[7], box[5]),
            // Ground plane
            PLANE(box[1], box[5], box[4], box[0]),
            // Left plane
            PLANE(box[0], box[2], box[3], box[1]),
            // Right plane
            PLANE(box[5], box[7], box[6], box[4]),
            // Top plane
            PLANE(box[3], box[2], box[6], box[7]),
            // Lighting plane top
            {.v1 = {-0.1f, 0.499f, 0.6f}, .v2 = {-0.1f, 0.499f, 0.4f}, .v3 = {0.1f, 0.499f, 0.4f}},
            {.v1 = {-0.1f, 0.499f, 0.6f}, .v2 = {0.1f, 0.499f, 0.4f}, .v3 = {0.1f, 0.499f, 0.6f}},
            // Lighting plane left
            {.v1 = {-0.499f, 0.2f, 0.2f}, .v2 = {-0.499f, 0.25f, 0.2f}, .v3 = {-0.499f, 0.25f, 0.8f}},
            {.v1 = {-0.499f, 0.25f, 0.8f}, .v2 = {-0.499f, 0.2f, 0.8f}, .v3 = {-0.499f, 0.2f, 0.2f}},
            // Right shadow caster plane
            {(Vec3){0.25f, -0.45f, 1.0f}, (Vec3){0.25f, 0.45f, 1.0f}, (Vec3){0.25f, 0.45f, 0.3f}},
            {(Vec3){0.25f, 0.45f, 0.3f}, (Vec3){0.25f, -0.45f, 0.3f}, (Vec3){0.25f, -0.45f, 1.0f}},
            // Center shadow caster plane
            {(Vec3){0.15f, 0.05f, 0.7f}, (Vec3){0.15f, 0.15f, 0.7f}, (Vec3){0.15f, 0.15f, 0.5f}},
            {(Vec3){0.15f, 0.15f, 0.5f}, (Vec3){0.15f, 0.05f, 0.5f}, (Vec3){0.15f, 0.05f, 0.7f}},
            // Center shadow caster plane 2
            {(Vec3){0.1f, -0.15f, 0.7f}, (Vec3){0.1f, 0.1f, 0.7f}, (Vec3){0.1f, 0.1f, 0.5f}},
            {(Vec3){0.1f, 0.1f, 0.5f}, (Vec3){0.1f, -0.15f, 0.5f}, (Vec3){0.1f, -0.15f, 0.7f}},
        },
        .triangle_material_handles = {
            1, 1,
            1, 1,
            2, 2,
            3, 3,
            1, 1,
            4, 4,
            5, 5,
            1, 1,
            6, 6,
            3, 3,
        },
        .triangle_count = 20
    };
    unsigned int spp_input = 32;
    if(argc > 1) {
        spp_input = atoi(argv[1]);
    }
    const unsigned int spp = spp_input;
    const double report_timer_interval_s = 1.0;

    const unsigned int IMAGE_DATA_SIZE = IMAGE_SIZE_X * IMAGE_SIZE_Y * 3;
    unsigned char data[IMAGE_DATA_SIZE];

    printf("Begin sampling\n");
    const clock_t start = clock();
    clock_t report_timer_start = clock();
    unsigned int data_filled = 0;
    const unsigned int data_report_interval = 1000000 / spp;

    #pragma omp parallel for
    for(unsigned int y = 0; y < IMAGE_SIZE_Y; y++) {
        for(unsigned int x = 0; x < IMAGE_SIZE_X; x++) {
            const unsigned int data_index = (x+y*IMAGE_SIZE_X)*3;
            
            const Color3f pixel_color = samplePixelColor(&scene, x, y, spp);
            data[data_index + 0] = (char)(clamp(pixel_color.r, 0.0f, 1.0f) * 255.0f);
            data[data_index + 1] = (char)(clamp(pixel_color.g, 0.0f, 1.0f) * 255.0f);
            data[data_index + 2] = (char)(clamp(pixel_color.b, 0.0f, 1.0f) * 255.0f);
            const unsigned int progress = ++data_filled; 
            if(progress % data_report_interval == data_report_interval - 1) {
                const double work_done = ((double)(progress * 3) / (double)(IMAGE_DATA_SIZE));
                const clock_t end = clock();
                const double cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
                const double approx_total_time = cpu_time_used / work_done;
                printf("Finished %4.1f%%, %02d:%02d / ~%02d:%02d\n", work_done * 100.0, (unsigned int)(cpu_time_used)/60, (unsigned int)(cpu_time_used)%60, (unsigned int)(approx_total_time)/60, (unsigned int)(approx_total_time)%60);
            }
            
            /*
            const clock_t report_timer_end = clock();
            const double timer_s = ((double) (report_timer_end - report_timer_start)) / CLOCKS_PER_SEC;

            if(timer_s > report_timer_interval_s) {
                const double work_done = ((double)(data_index) / (double)(IMAGE_DATA_SIZE));
                const clock_t end = clock();
                const double cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
                const double approx_total_time = cpu_time_used / work_done;
                printf("Finished %4.1f%%, %02d:%02d / ~%02d:%02d\n", work_done * 100.0, (unsigned int)(cpu_time_used)/60, (unsigned int)(cpu_time_used)%60, (unsigned int)(approx_total_time)/60, (unsigned int)(approx_total_time)%60);
                // reset timer
                report_timer_start = clock();
            }
            */
        }
    }
    const clock_t end = clock();
    const double cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("Finished sampling\n");

    char filename[256] = ".\\out\\render_";
    char temp_convert[32];
    snprintf(temp_convert, 32,"%d", time(NULL));
    strcat(filename, temp_convert);
    strcat(filename, "_");
    snprintf(temp_convert, 32,"%d", spp);
    strcat(filename, temp_convert);
    strcat(filename, ".ppm");

    write_ppm(filename, IMAGE_SIZE_X, IMAGE_SIZE_Y, data);
    printf("Wrote image to '%s'\n", filename);

    const Stats gs = global_stats;
    const unsigned int bounce_rays_count = gs.ray_count - gs.primary_rays.count;
    const unsigned int bounce_rays_hits = gs.ray_hits - gs.primary_rays.hits;

    #if STATS
    printf("Statistics\n");
    printf("==========\n");
    printf("GENERAL\n");
    printStatTime("Total time", (unsigned int)(cpu_time_used));
    printStatTotal("Total rays", gs.ray_count);
    printStatFactor("Rays per second", (double)(gs.ray_count) / cpu_time_used);
    printf("PRIMARY RAYS\n");
    printStatTotal("Total primary rays", gs.primary_rays.count);
    printStatTotalPercent("Primary ray hits", gs.primary_rays.hits, gs.primary_rays.count);
    printStatTotalPercent("Primary rays to light source", gs.primary_rays.hit_emissive, gs.primary_rays.count);
    printf("BOUNCE RAYS\n");
    printStatTotal("Total bounce rays", bounce_rays_count);
    printStatTotalPercent("Bounce ray hits", bounce_rays_hits, bounce_rays_count);
    printStatTotalPercent("Bounce rays to light source", gs.bounce_rays.hit_emissive, bounce_rays_count);
    printStatTotalPercent("Bounce rays with max depth", gs.bounce_rays.reached_max_depth, bounce_rays_count);
    printStatFactor("Avg bounce ray depth", ((double)(bounce_rays_count)/ (double)(spp)) / ((double)(gs.primary_rays.hits)));
    #endif
    
    return 0;
}