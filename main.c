#include "platform.h"
#include "graphics.h"
#include "mem.h"

#include <stdio.h>
#include <string.h>

int main()
{
    const int INSTS_SIZE = PLATFORM_T_SIZE + GRAPHICS_T_SIZE;

    graphicsplatform_t graphics_platform;
    ut_zero(graphics_platform);

    ut_dynsalloc(char, insts, INSTS_SIZE);
    memset(insts, 0, INSTS_SIZE);

    platform_t* const platform = (platform_t*)(insts);
    graphics_t* const graphics = (graphics_t*)(insts + PLATFORM_T_SIZE);

    if (!PLT_startup(platform, &graphics_platform))
    {
        fprintf(stderr, "Failed initializing platform!\n");
        return -1;
    }

    if (!GPH_startup(graphics, &graphics_platform))
    {
        fprintf(stderr, "Failed initializing graphics!\n");
        return -1;
    }

    return PLT_run(platform, graphics);
}