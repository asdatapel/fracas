#pragma once

#include "util.hpp"

// TODO: maybe each allocator should be with its subsystem

StackAllocator assets_allocator;
StackAllocator assets_temp_allocator;
Memory assets_memory = {&assets_allocator, &assets_temp_allocator};

StackAllocator scene_allocator;
StackAllocator scene_temp_allocator;
Memory scene_memory = {&scene_allocator, &scene_temp_allocator};