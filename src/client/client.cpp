#include "client.hpp"

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>

Client::Client() {
    m_window = new Window(1280, 720, "Voksel");
    m_window->vsync(true);
    m_window->disable_cursor();

    if (!gladLoadGLLoader(m_window->loadproc())) {
        throw std::runtime_error("Failed to initialize GLAD");
    }

    m_window->set_framebuffer_size_callback([&] (uint width, uint height) {
        glViewport(0, 0, width, height);
        m_renderer->on_framebuffer_resize(width, height);
    });

    m_window->set_cursor_pos_callback([&] (f64 x, f64 y) {
        if (!m_window->is_cursor_enabled())
            m_renderer->on_cursor_move(x, y);
    });

    m_camera = new Camera(glm::vec3(0, 0, -2));
    m_camera->set_free(true);
    m_renderer = new Renderer(m_window, m_camera);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.IniFilename = nullptr;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(m_window->glfw_window(), true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    m_world = new World();
    std::cout << m_world->get_block_at(glm::i32vec3(300, 42, 54)) << std::endl;
    m_world->set_block_at(glm::i32vec3(300, 42, 54), 2);
    std::cout << m_world->get_block_at(glm::i32vec3(300, 42, 54)) << std::endl;
}

Client::~Client() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void Client::loop() {
    m_window->loop([this] {
        f64 current_frame = glfwGetTime();
        m_delta_time = current_frame - m_last_frame;
        m_last_frame = current_frame;

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGui::Begin("voksel", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Text("hello ><>");
        ImGui::End();

        ImGui::Render();

        process_input();

        m_renderer->render();

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    });
}

void Client::process_input() {
    if (m_window->is_key_pressed(GLFW_KEY_ESCAPE))
        m_window->close();
    
    static bool tab_locked = false;
    if (m_window->is_key_pressed(GLFW_KEY_TAB)) {
        if (!tab_locked) {
            m_window->toggle_cursor();
            tab_locked = true;
        }
    } else tab_locked = false;

    static bool f_locked = false;
    if (m_window->is_key_pressed(GLFW_KEY_F)) {
        if (!f_locked) {
            m_camera->set_free(!m_camera->is_free());
            f_locked = true;
        }
    } else f_locked = false;

    if (m_camera->is_free()) {
        if (m_window->is_key_pressed(GLFW_KEY_W))
            m_camera->process_free_movement(MovementDirection::FORWARD, m_delta_time);
        if (m_window->is_key_pressed(GLFW_KEY_A))
            m_camera->process_free_movement(MovementDirection::LEFT, m_delta_time);
        if (m_window->is_key_pressed(GLFW_KEY_S))
            m_camera->process_free_movement(MovementDirection::BACKWARD, m_delta_time);
        if (m_window->is_key_pressed(GLFW_KEY_D))
            m_camera->process_free_movement(MovementDirection::RIGHT, m_delta_time);
    }
}
