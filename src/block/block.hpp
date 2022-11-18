#pragma once

#include <string>

#include "../util.hpp"

using BlockNID = u32;

class Block {
    std::string m_id;

public:
    Block(std::string id);
};
