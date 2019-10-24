#ifndef STATS_H
#define STATS_H

#include <stdio.h>

#if STATS
typedef struct Stats {
    unsigned int ray_count;
    unsigned int ray_hits;
    struct PrimaryRays {
        unsigned int count;
        unsigned int hits;
        unsigned int hit_emissive;
    } primary_rays;
    struct BounceRays {
        unsigned int reached_max_depth;
        unsigned int hit_emissive;
    } bounce_rays;
} Stats;

void printStatTotal(const char* text, unsigned int value) {
    printf("  %-40s%16d\n", text, value);
}

void printStatFactor(const char* text, double value) {
    printf("  %-40s%16.2f\n", text, value);
}

void printStatTime(const char* text, unsigned int seconds) {
    printf("  %-40s%13d:%02d\n", text, seconds/60, seconds%60);
}

void printStatTotalPercent(const char* text, unsigned int value, unsigned int percent_100) {
    printf("  %-40s%16d    %4.1f%%\n", text, value, ((double)value / (double)percent_100) * 100.0);
}

#endif


#endif // STATS_H