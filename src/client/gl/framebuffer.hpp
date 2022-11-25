#pragma once

#include "../../util.hpp"

class Framebuffer {
    uint m_id;
    uint m_color_buffer;
    uint m_renderbuffer;
    uint m_width, m_height;

public:
    Framebuffer(uint width, uint height);
    ~Framebuffer();
    
    void bind();
    static void unbind();
    void bind_color_buffer();
    void resize(uint width, uint height);
};
