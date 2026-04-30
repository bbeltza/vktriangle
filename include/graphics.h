#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _graphics_t graphics_t;
extern const int GRAPHICS_T_SIZE;
extern float DELTA_TIME;

typedef struct _graphicsplatform_t graphicsplatform_t;

#if _WIN32
    struct _graphicsplatform_t
    {
        void* hWnd;
        void* hInstance;
    };
#endif

extern void GPH_render(graphics_t* inst);
extern void GPH_close(graphics_t* inst);
extern void GPH_size(graphics_t* inst, int x, int y);
extern bool GPH_startup(graphics_t* inst, const graphicsplatform_t* platform);

#ifdef __cplusplus
}
#endif

#endif