#include "world.hpp"

constexpr i32 noise_factor = 2;
constexpr i32 noise_size = Chunk::size / noise_factor + 1;
constexpr i32 noise_area = noise_size * noise_size;

void World::generate_noise(std::array<f32, Chunk::area> &out, i32 x_start, i32 z_start) {
    constexpr f32 frequency = 0.005f;
    std::array<f32, noise_area> noise_output;
    m_max_smooth->GenUniformGrid2D(noise_output.data(), x_start / 2, z_start / 2, noise_size, noise_size, frequency * 2.0f, m_seed);
    for (i32 z = 0; z < Chunk::size; z++) {
        for (i32 x = 0; x < Chunk::size; x++) {
            f32 value = 0;
            if (x % 2 == 0 && z % 2 == 0) {
                value = noise_output[(z / 2) * noise_size + x / 2];
            } else if (x % 2 == 0 && z % 2 != 0) {
                value = (noise_output[(z / 2) * noise_size + x / 2] + noise_output[(z / 2 + 1) * noise_size + x / 2]) / 2.0f;
            } else if (x % 2 != 0 && z % 2 == 0) {
                value = (noise_output[(z / 2) * noise_size + x / 2] + noise_output[(z / 2) * noise_size + x / 2 + 1]) / 2.0f;
            } else {
                f32 value1 = (noise_output[(z / 2) * noise_size + x / 2] + noise_output[(z / 2) * noise_size + x / 2 + 1]) / 2.0f;
                f32 value2 = (noise_output[(z / 2 + 1) * noise_size + x / 2] + noise_output[(z / 2 + 1) * noise_size + x / 2 + 1]) / 2.0f;
                value = (value1 + value2) / 2.0f;
            }
            out[z * Chunk::size + x] = value;
        }
    }
}

void World::shape_chunk(i32vec3 chunk_pos) {
    Chunk *chunk = get_chunk(chunk_pos);
    auto world_x = chunk_pos.x * Chunk::size;
    auto world_y = chunk_pos.y * Chunk::size;
    auto world_z = chunk_pos.z * Chunk::size;

    std::array<f32, Chunk::area> noise_output;
    generate_noise(noise_output, world_x, world_z);

    std::array<f32, Chunk::volume> cave_noise_output = {};
    if (world_y <= 32)
        m_cave_noise->GenUniformGrid3D(cave_noise_output.data(), world_x, world_y, world_z, Chunk::size, Chunk::size, Chunk::size, 0.019f, m_seed);

    for (i32 x = 0; x < Chunk::size; x++) {
        for (i32 z = 0; z < Chunk::size; z++) {
            i32 height = std::floor(noise_output[z * Chunk::size + x] * 32.0f);
            for (i32 y = 0; y < Chunk::size; y++) {
                BlockNID block = 0;
                if (world_y > 1 && world_y == height) {
                    block = 4;
                } else if (world_y > 0 && world_y == height - 1) {
                    block = 3;
                } else {
                    if (world_y == height || world_y == height - 1)
                        block = 7;
                    if (world_y <= 0 && world_y > height)
                        block = 10;
                }
                if (world_y <= height) {
                    if (world_y > 32 && world_y < height - 1) {
                        block = 1;
                    } else {
                        f32 cave_multiplier = 50.0f - 25.0f * std::clamp((world_y + 32.0f) / 64.0f, 0.0f, 1.0f);
                        f32 cave_density = (cave_noise_output[z * Chunk::size * Chunk::size + y * Chunk::size + x] + 1.0f) * cave_multiplier;
                        if (world_y < height - 1 && cave_density < 75.0f)
                            block = 1;
                        else if (cave_density >= 75.0f)
                            block = 0;
                    }
                }

                if (block != 0)
                    chunk->set_block_at(i32vec3(x, y, z), block);

                world_y++;
            }
            world_y -= Chunk::size;
            world_z++;
        }
        world_z -= Chunk::size;
        world_x++;
    }
}

usize World::pos_hash(i32vec3 world_pos) {
    return std::hash<i32vec4>()(i32vec4(world_pos, m_seed));
}

void World::generate_tree(i32vec3 world_pos) {
    int length_roll = pos_hash(world_pos + i32vec3(0, 1, 0)) % 100;
    int length = 5;
    if (length_roll > 50)
        length = 6;
    if (length_roll > 80)
        length = 7;

    // generate trunk
    for (int y = 0; y < length; y++)
        set_block_at(world_pos + i32vec3(0, y, 0), 6);

    // generate leaves
    for (int x = -2; x < 3; x++) {
        for (int z = -2; z < 3; z++) {
            if (x == 0 && z == 0)
                continue;
            set_block_at(world_pos + i32vec3(x, length - 2, z), 12);
        }
    }
    for (int x = -2; x < 3; x++) {
        for (int z = -2; z < 3; z++) {
            if (x == 0 && z == 0)
                continue;
            if ((x == -2 && z == -2) || (x == 2 && z == -2) || (x == -2 && z == 2) || (x == 2 && z == 2))
                if (pos_hash(world_pos + i32vec3(x, length - 1, z)) % 100 > 25)
                    continue;
            set_block_at(world_pos + i32vec3(x, length - 1, z), 12);
        }
    }
    for (int x = -1; x < 2; x++)
        for (int z = -1; z < 2; z++)
            set_block_at(world_pos + i32vec3(x, length, z), 12);
    for (int x = -1; x < 2; x++) {
        for (int z = -1; z < 2; z++) {
            if ((x == -1 && z == -1) || (x == 1 && z == -1) || (x == -1 && z == 1) || (x == 1 && z == 1))
                if (pos_hash(world_pos + i32vec3(x, length + 1, z)) % 100 > 25)
                    continue;
            set_block_at(world_pos + i32vec3(x, length + 1, z), 12);
        }
    }
}

void World::decorate_chunk(i32vec3 chunk_pos) {
    auto world_x = chunk_pos.x * Chunk::size;
    auto world_y = chunk_pos.y * Chunk::size;
    auto world_z = chunk_pos.z * Chunk::size;

    std::array<f32, Chunk::area> noise_output;
    generate_noise(noise_output, world_x, world_z);

    for (i32 x = 0; x < Chunk::size; x++) {
        for (i32 z = 0; z < Chunk::size; z++) {
            i32 height = std::floor(noise_output[z * Chunk::size + x] * 32.0f);
            for (i32 y = 0; y < Chunk::size; y++) {
                if (world_y > 2 && world_y == height + 1) {
                    i32vec3 world_pos(world_x, world_y, world_z);
                    int decoration_roll = pos_hash(world_pos) % 100;
                    if (decoration_roll > 75) // generate grass
                        set_block_at(world_pos, 11);
                    if (decoration_roll == 75) // generate dandelions
                        set_block_at(world_pos, 13);
                    if (decoration_roll == 74) // generate roses
                        set_block_at(world_pos, 14);
                    if (decoration_roll == 73) // generate trees
                        generate_tree(world_pos);
                }
                world_y++;
            }
            world_y -= Chunk::size;
            world_z++;
        }
        world_z -= Chunk::size;
        world_x++;
    }

    get_chunk(chunk_pos)->m_decorated.test_and_set();
}
