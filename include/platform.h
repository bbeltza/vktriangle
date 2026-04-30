#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _platform_t platform_t;
struct _graphicsplatform_t;
struct _graphics_t;

extern const int PLATFORM_T_SIZE;

extern bool PLT_startup(platform_t* plt, struct _graphicsplatform_t* gph_plt);
extern int PLT_run(platform_t* plt, struct _graphics_t* gph);

#ifdef __cplusplus
}
#endif

#endif