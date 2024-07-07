#include "player.hpp"

vec3 resolve_movement(World *world, vec3 old_pos, vec3 new_pos, vec3 min_bounds, vec3 max_bounds, vec3 *velocity) {
    if (world->is_position_valid(new_pos, min_bounds, max_bounds))
        return new_pos;

    vec3 return_pos = old_pos;
    vec3 new_velocity = velocity ? *velocity : vec3(0);

    if (world->is_position_valid(vec3(return_pos.x, new_pos.y, return_pos.z), min_bounds, max_bounds)) {
        return_pos.y = new_pos.y;
    } else {
        if (new_pos.y > old_pos.y)
            return_pos.y += glm::ceil(old_pos.y + max_bounds.y) - old_pos.y - max_bounds.y - 0.0001f;
        else
            return_pos.y -= old_pos.y + min_bounds.y - glm::floor(old_pos.y + min_bounds.y) - 0.0001f;
        new_velocity.y = 0;
    }

    if (world->is_position_valid(vec3(new_pos.x, return_pos.y, return_pos.z), min_bounds, max_bounds)) {
        return_pos.x = new_pos.x;
    } else {
        if (new_pos.x > old_pos.x)
            return_pos.x += glm::ceil(old_pos.x + max_bounds.x) - old_pos.x - max_bounds.x - 0.0001f;
        else
            return_pos.x -= old_pos.x + min_bounds.x - glm::floor(old_pos.x + min_bounds.x) - 0.0001f;
        new_velocity.x = 0;
    }

    if (world->is_position_valid(vec3(return_pos.x, return_pos.y, new_pos.z), min_bounds, max_bounds)) {
        return_pos.z = new_pos.z;
    } else {
        if (new_pos.z > old_pos.z)
            return_pos.z += glm::ceil(old_pos.z + max_bounds.z) - old_pos.z - max_bounds.z - 0.0001f;
        else
            return_pos.z -= old_pos.z + min_bounds.z - glm::floor(old_pos.z + min_bounds.z) - 0.0001f;
        new_velocity.z = 0;
    }

    if (velocity)
        *velocity = new_velocity;
    return return_pos;
}
