#include "graphics.h"
#include "mem.h"

// Shader headers
#include "fs.h"
#include "vs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#if _WIN32
    #define VK_USE_PLATFORM_WIN32_KHR
#elif _XLIB
    #define VK_USE_PLATFORM_XLIB_KHR
#endif

#include <vulkan/vulkan.h>

#define USE_SRGB 1

#define VKFRAMEBUFFER_COUNT 2
#if USE_SRGB
    #define VKFRAMEBUFFER_FORMAT VK_FORMAT_B8G8R8A8_SRGB
#else
    #define VKFRAMEBUFFER_FORMAT VK_FORMAT_B8G8R8A8_UNORM
#endif
struct _graphics_t
{
    VkInstance vkinstance;
    VkDevice vkdevice;
    VkCommandPool vkcmdpool;
    VkQueue vkqueue;

    VkSemaphore vksemaphores[2];
    VkCommandBuffer vkcmdbuffers[1];

    VkShaderModule vkvertshader;
    VkShaderModule vkfragshader;
    VkRenderPass vkrenderpass;
    VkPipelineLayout vkpipelinelayout;
    VkPipeline vkpipeline;

    VkSurfaceKHR vksurface;
    VkSwapchainKHR vkswapchain;

    VkFramebuffer vkframebuffers[VKFRAMEBUFFER_COUNT];
    VkImageView vkframebufferviews[VKFRAMEBUFFER_COUNT];

    VkExtent2D vkscreenextent;
    float vkpushconstantangle;
    float vkpushconstantaspect;

    bool wantresize;
};
const int GRAPHICS_T_SIZE = sizeof(graphics_t);

static VkBool32 VKAPI_CALL vk_dbgusercallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT             messageTypes,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void*                                       pUserData)
{
    fprintf(stderr, "[Vulkan][%s]: %s\n", pCallbackData->pMessageIdName, pCallbackData->pMessage);
    return VK_FALSE;
}

static bool vk_checklayer(const char* layer)
{
    VkResult vr;
    uint32_t propertycount;

    vr = vkEnumerateInstanceLayerProperties(&propertycount, NULL);
    if (vr != VK_SUCCESS)
        return false;

    if (!propertycount)
        return false;

    ut_dynsalloc(VkLayerProperties, layers, propertycount);
    vr = vkEnumerateInstanceLayerProperties(&propertycount, layers);
    if (vr != VK_SUCCESS)
        return false;

    for (uint32_t i = 0; i < propertycount; i++)
    {
        if (strcmp(layers[i].layerName, layer) == 0)
            return true;
    }

    return false;
}

static void vk_releaseframebuffers(graphics_t* inst)
{
    if (!inst->vkswapchain)
        return;
    
    assert(inst->vkswapchain != NULL);
    
    for (int i = 0; i < VKFRAMEBUFFER_COUNT; i++)
    {
        assert(inst->vkframebuffers[i] != NULL);
        assert(inst->vkframebufferviews[i] != NULL);

        vkDestroyImageView(inst->vkdevice, inst->vkframebufferviews[i], NULL);
        vkDestroyFramebuffer(inst->vkdevice, inst->vkframebuffers[i], NULL);
    }
    vkDestroySwapchainKHR(inst->vkdevice, inst->vkswapchain, NULL);
}

static bool vk_createswapchain(graphics_t* inst)
{
    VkResult vr;
    (void)vr;

    const int w = inst->vkscreenextent.width;
    const int h = inst->vkscreenextent.height;

    vr = vkCreateSwapchainKHR(inst->vkdevice, &(VkSwapchainCreateInfoKHR){
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .flags = 0,
        .surface = inst->vksurface,
        .minImageCount = ut_arrcount(inst->vkframebuffers),
        .imageFormat = VKFRAMEBUFFER_FORMAT,
        .imageExtent = { .width = w, .height = h },
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                      VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = VK_PRESENT_MODE_FIFO_KHR,
        .clipped = VK_TRUE,
        .oldSwapchain = VK_NULL_HANDLE
    }, NULL, &inst->vkswapchain);
    if (vr != VK_SUCCESS)
        return false;
        
    VkImage swimgs[VKFRAMEBUFFER_COUNT];
    uint32_t count = VKFRAMEBUFFER_COUNT;
    vr = vkGetSwapchainImagesKHR(inst->vkdevice, inst->vkswapchain, &count, swimgs);
    if (vr != VK_SUCCESS)
        return false;

    for (int i = 0; i < VKFRAMEBUFFER_COUNT; i++)
    {
        vr = vkCreateImageView(inst->vkdevice, &(VkImageViewCreateInfo){
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = swimgs[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = VKFRAMEBUFFER_FORMAT,
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        }, NULL, &inst->vkframebufferviews[i]);

        vr = vkCreateFramebuffer(inst->vkdevice, &(VkFramebufferCreateInfo){
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = inst->vkrenderpass,
            .width = w,
            .height = h,
            .pAttachments = &inst->vkframebufferviews[i],
            .attachmentCount = 1,
            .layers = 1
        }, NULL, &inst->vkframebuffers[i]);

        if (vr != VK_SUCCESS)
            return false;
    }

    inst->wantresize = false;
    return true;
}

bool GPH_startup(graphics_t* inst, const graphicsplatform_t* platform)
{
    VkResult vr;
    (void)vr;

    {
        #ifndef NDEBUG
            VkDebugUtilsMessengerCreateInfoEXT dbginfo = {
                .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
                .flags = 0,
                .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
                                VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT,
                .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
                .pfnUserCallback = vk_dbgusercallback,
                .pUserData = inst
            };
        #endif

        VkApplicationInfo appinfo = {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pApplicationName = "Vulkan App",
            .apiVersion = VK_API_VERSION_1_0,
            .applicationVersion = VK_MAKE_VERSION(0, 1, 0)
        };
        VkInstanceCreateInfo createinfo = {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pApplicationInfo = &appinfo
        };
        
        createinfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createinfo.pApplicationInfo = &appinfo;
        #ifndef NDEBUG
            static const char* const LAYERS[] =  {
                "VK_LAYER_KHRONOS_validation"
            };

            createinfo.pNext = &dbginfo;
            // Check if the layer is available
            if (vk_checklayer(LAYERS[0]))
            {
                createinfo.ppEnabledLayerNames = LAYERS;
                createinfo.enabledLayerCount = 1;
            }
        #endif
        static const char* const EXTENSION_NAMES[] = {
            VK_KHR_SURFACE_EXTENSION_NAME,
            #ifndef NDEBUG
                VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
            #endif
            #if _WIN32
                VK_KHR_WIN32_SURFACE_EXTENSION_NAME
            #elif _XLIB
                VK_KHR_XLIB_SURFACE_EXTENSION_NAME
            #endif
        };

        createinfo.ppEnabledExtensionNames = EXTENSION_NAMES;
        createinfo.enabledExtensionCount = ut_arrcount(EXTENSION_NAMES);

        vr = vkCreateInstance(&createinfo, NULL, &inst->vkinstance);
        if (vr != VK_SUCCESS)
            return false;
    }

#if _WIN32
    vr = vkCreateWin32SurfaceKHR(inst->vkinstance, &(VkWin32SurfaceCreateInfoKHR){
        .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
        .hwnd = platform->hWnd,
        .hinstance = platform->hInstance
    }, NULL, &inst->vksurface);
#elif _XLIB
    vr = vkCreateXlibSurfaceKHR(inst->vkinstance, &(VkXlibSurfaceCreateInfoKHR){
        .sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR,
        .dpy = platform->display,
        .window = (Window)platform->window
    }, NULL, &inst->vksurface);
#endif
    if (vr != VK_SUCCESS)
        return false;

    {
        // Just get the first device from vkEnumeratePhysicalDevices ; I might want to enumerate every single device to find the most suitable one 
        VkPhysicalDevice pfirstdev;
        uint32_t pdevcount = 1;
        vr = vkEnumeratePhysicalDevices(inst->vkinstance, &pdevcount, &pfirstdev);

        #if 01
        {
            VkSurfaceCapabilitiesKHR capabilities;
            vr = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pfirstdev, inst->vksurface, &capabilities);
            if (vr == VK_SUCCESS)
            {
                inst->vkscreenextent = capabilities.currentExtent;
                inst->wantresize = true;
            }
        }
        #endif

        vr = vkCreateDevice(pfirstdev, &(VkDeviceCreateInfo){
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .queueCreateInfoCount = 1,
            .enabledExtensionCount = 1,
            .ppEnabledExtensionNames = (const char*[]){ VK_KHR_SWAPCHAIN_EXTENSION_NAME },
            .pQueueCreateInfos = (VkDeviceQueueCreateInfo[]){
                {
                    .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                    .queueCount = 1,
                    .pQueuePriorities = (float[]){ 1.0f },

                }
            },
        }, NULL, &inst->vkdevice);
        if (vr != VK_SUCCESS)
            return false;
        
        vkGetDeviceQueue(inst->vkdevice, 0, 0, &inst->vkqueue);
    }


    vr = vkCreateCommandPool(inst->vkdevice, &(VkCommandPoolCreateInfo){
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT
    }, NULL, &inst->vkcmdpool);
    if (vr != VK_SUCCESS)
        return false;

    vr = vkAllocateCommandBuffers(inst->vkdevice, &(VkCommandBufferAllocateInfo){
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = inst->vkcmdpool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = ut_arrcount(inst->vkcmdbuffers)
    }, inst->vkcmdbuffers);
    if (vr != VK_SUCCESS)
        return false;

    //

    vr = vkCreateRenderPass(inst->vkdevice, &(VkRenderPassCreateInfo){
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .subpassCount = 1,
        .attachmentCount = 1,
        .pSubpasses = (VkSubpassDescription[]){
            {
                .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
                .colorAttachmentCount = 1,
                .pColorAttachments = (VkAttachmentReference[]){{
                    .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
                }}
            }
        },
        .pAttachments = (VkAttachmentDescription[]){
            {
                .samples = VK_SAMPLE_COUNT_1_BIT,
                .format = VKFRAMEBUFFER_FORMAT,
                .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE
            }
        }
    }, NULL, &inst->vkrenderpass);

    if (vr != VK_SUCCESS)
        return false;

    //

    vr = vkCreatePipelineLayout(inst->vkdevice, &(VkPipelineLayoutCreateInfo){
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .flags = 0,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = (VkPushConstantRange[]){
            {
                .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
                .offset = 0,
                .size = sizeof(float)*2
            }
        }
    }, NULL, &inst->vkpipelinelayout);

    vr = vkCreateShaderModule(inst->vkdevice, &(VkShaderModuleCreateInfo){
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .flags = 0,
        .pCode = (void*)g_VSmain,
        .codeSize = sizeof(g_VSmain)
    }, NULL, &inst->vkvertshader);
    vr = vkCreateShaderModule(inst->vkdevice, &(VkShaderModuleCreateInfo){
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .flags = 0,
        .pCode = (void*)g_PSmain,
        .codeSize = sizeof(g_PSmain)
    }, NULL, &inst->vkfragshader);

    vr = vkCreateGraphicsPipelines(inst->vkdevice, VK_NULL_HANDLE, 1, (VkGraphicsPipelineCreateInfo[]){
        {
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .flags = 0,
            .stageCount = 2,
            .pStages = (VkPipelineShaderStageCreateInfo[]){
                { // Vertex shader
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                    .flags = 0,
                    .stage = VK_SHADER_STAGE_VERTEX_BIT,
                    .pName = "VSmain",
                    .module = inst->vkvertshader
                },
                { // Fragment/pixel shader
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                    .flags = 0,
                    .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
                    .pName = "PSmain",
                    .module = inst->vkfragshader
                }
            },
            .pVertexInputState = &(VkPipelineVertexInputStateCreateInfo){
                .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
                .flags = 0,
                // The triangle data is stored inside the shader
                .vertexBindingDescriptionCount = 0,
                .vertexAttributeDescriptionCount = 0
            },
            .pInputAssemblyState = &(VkPipelineInputAssemblyStateCreateInfo){
                .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
                .flags = 0,
                .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
                .primitiveRestartEnable = VK_FALSE
            },
            .pTessellationState = &(VkPipelineTessellationStateCreateInfo){
                .sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
                .flags = 0,
                .patchControlPoints = 0
            },
            .pViewportState = &(VkPipelineViewportStateCreateInfo){
                .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
                .flags = 0,
                .viewportCount = 1,
                .scissorCount = 1
            },
            .pRasterizationState = &(VkPipelineRasterizationStateCreateInfo){
                .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
                .flags = 0,
                .polygonMode = VK_POLYGON_MODE_FILL,
                .cullMode = VK_CULL_MODE_NONE,
                .lineWidth = 1
            },
            .pMultisampleState = &(VkPipelineMultisampleStateCreateInfo){
                .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
                .rasterizationSamples = 1
            },
            .pColorBlendState = &(VkPipelineColorBlendStateCreateInfo){
                .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
                .flags = 0,
                .attachmentCount = 1,
                .pAttachments = (VkPipelineColorBlendAttachmentState[]){
                    {
                        .blendEnable = VK_FALSE,
                        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                                          VK_COLOR_COMPONENT_G_BIT |
                                          VK_COLOR_COMPONENT_B_BIT |
                                          VK_COLOR_COMPONENT_A_BIT
                    }
                }
            },
            .pDynamicState = &(VkPipelineDynamicStateCreateInfo){
                .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
                .flags = 0,
                .dynamicStateCount = 2,
                .pDynamicStates = (VkDynamicState[]){
                    VK_DYNAMIC_STATE_VIEWPORT,
                    VK_DYNAMIC_STATE_SCISSOR
                }
            },
            .layout = inst->vkpipelinelayout,
            .renderPass = inst->vkrenderpass
        }
    }, NULL, &inst->vkpipeline);
    if (vr != VK_SUCCESS)
        return false;

    {
        VkSemaphoreCreateInfo createinfo = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            .flags = 0
        };
        vr = vkCreateSemaphore(inst->vkdevice, &createinfo, NULL, &inst->vksemaphores[0]);
        vr = vkCreateSemaphore(inst->vkdevice, &createinfo, NULL, &inst->vksemaphores[1]);
        if (vr != VK_SUCCESS)
            return false;
    }

    if (inst->wantresize)
        vk_createswapchain(inst);

    return true;
}

void GPH_close(graphics_t* inst)
{
    vk_releaseframebuffers(inst);

    for (int i = 0; i < ut_arrcount(inst->vksemaphores); i++)
        vkDestroySemaphore(inst->vkdevice, inst->vksemaphores[i], NULL);

    vkDestroyShaderModule(inst->vkdevice, inst->vkvertshader, NULL);
    vkDestroyShaderModule(inst->vkdevice, inst->vkfragshader, NULL);
    vkDestroyPipeline(inst->vkdevice, inst->vkpipeline, NULL);
    vkDestroyPipelineLayout(inst->vkdevice, inst->vkpipelinelayout, NULL);
    vkDestroyRenderPass(inst->vkdevice, inst->vkrenderpass, NULL);
    vkDestroyCommandPool(inst->vkdevice, inst->vkcmdpool, NULL);
    vkDestroyDevice(inst->vkdevice, NULL);

    vkDestroySurfaceKHR(inst->vkinstance, inst->vksurface, NULL);
    vkDestroyInstance(inst->vkinstance, NULL);
}

void GPH_render(graphics_t* inst)
{
    VkResult vr;
    (void)vr;

    vr = vkQueueWaitIdle(inst->vkqueue);

    if (inst->wantresize)
    {
        vk_releaseframebuffers(inst);
        if (!vk_createswapchain(inst))
            abort();
    }

    uint32_t img_index;
    vr = vkAcquireNextImageKHR(inst->vkdevice, inst->vkswapchain, __UINT64_MAX__, inst->vksemaphores[1], VK_NULL_HANDLE, &img_index);

    vr = vkResetCommandBuffer(inst->vkcmdbuffers[0], 0);
    vr = vkResetCommandPool(inst->vkdevice, inst->vkcmdpool, 0);

    vr = vkBeginCommandBuffer(inst->vkcmdbuffers[0], &(VkCommandBufferBeginInfo){
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    });

    vkCmdBeginRenderPass(inst->vkcmdbuffers[0], &(VkRenderPassBeginInfo){
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = inst->vkrenderpass,
        .framebuffer = inst->vkframebuffers[img_index],
        .clearValueCount = 1,
        .pClearValues = (VkClearValue[]){
            {
                .color = {
                    .float32 = { 0.5f, 0.5f, 1.0f, 1.0f }
                }
            }
        },
        .renderArea = { .extent = inst->vkscreenextent }
    }, VK_SUBPASS_CONTENTS_INLINE);

    { // All drawing comes in here
        vkCmdBindPipeline(inst->vkcmdbuffers[0], VK_PIPELINE_BIND_POINT_GRAPHICS, inst->vkpipeline);
        vkCmdSetScissor(inst->vkcmdbuffers[0], 0, 1, &(VkRect2D){ .extent = inst->vkscreenextent });
        vkCmdSetViewport(inst->vkcmdbuffers[0], 0, 1, &(VkViewport){
            .width = (float)inst->vkscreenextent.width,
            .height = (float)inst->vkscreenextent.height
        });
        vkCmdPushConstants(inst->vkcmdbuffers[0], inst->vkpipelinelayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(float)*2, &inst->vkpushconstantangle);
        vkCmdDraw(inst->vkcmdbuffers[0], 3, 1, 0, 0);
    }

    vkCmdEndRenderPass(inst->vkcmdbuffers[0]);
    vr = vkEndCommandBuffer(inst->vkcmdbuffers[0]);

    vr = vkQueueSubmit(inst->vkqueue, 1, &(VkSubmitInfo){
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pCommandBuffers = inst->vkcmdbuffers,
        .commandBufferCount = ut_arrcount(inst->vkcmdbuffers),
        .waitSemaphoreCount = 1,
        .signalSemaphoreCount = 1,
        .pWaitSemaphores = &inst->vksemaphores[1],
        .pSignalSemaphores = &inst->vksemaphores[0],
        .pWaitDstStageMask = &(VkPipelineStageFlags){VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT}
    }, NULL);

    vr = vkQueuePresentKHR(inst->vkqueue, &(VkPresentInfoKHR){
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .swapchainCount = 1,
        .waitSemaphoreCount = 1,
        .pSwapchains = &inst->vkswapchain,
        .pWaitSemaphores = &inst->vksemaphores[0],
        .pImageIndices = &img_index
    });

    inst->vkpushconstantangle += 0.05;
}

void GPH_size(graphics_t* inst, int w, int h)
{
    inst->vkscreenextent = (VkExtent2D){ .width = w, .height = h };
    // Aspect ratio constant for the shader, it's not used right now but it makes the triangle look equilateral
    inst->vkpushconstantaspect = (float)w / h;
    
    inst->wantresize = true;
}