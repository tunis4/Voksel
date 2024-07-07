#pragma once

#include "world/world.hpp"

vec3 resolve_movement(World *world, vec3 old_pos, vec3 new_pos, vec3 min_bounds, vec3 max_bounds, vec3 *velocity);
