#pragma once

#include "../../util.hpp"

class Framebuffer {
    uint m_id;
    uint m_color_buffer;
    uint m_renderbuffer;
    uint m_width, m_height;

public:
    Framebuffer(uint window_width, uint window_height);
    ~Framebuffer();
    
    void bind();
    static void unbind();
    void bind_color_buffer();
};
