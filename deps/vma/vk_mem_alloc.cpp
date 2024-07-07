#include <cstdio>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wparentheses"
#define VMA_IMPLEMENTATION
// #define VMA_DEBUG_LOG(...) printf(__VA_ARGS__); putchar('\n')
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#define VMA_VULKAN_VERSION 1002000 // Vulkan 1.2
#include "vk_mem_alloc.h"
#pragma GCC diagnostic pop
