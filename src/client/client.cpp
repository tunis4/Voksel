#include "client.hpp"

#include <filesystem>
#include <iostream>
#include <cmath>
#include <stdexcept>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>

Client::Client() {
    m_script_engine = new ScriptEngine();

    m_window = new Window(800, 600, "Voksel");
    m_window->vsync(true);

    if (!gladLoadGLLoader(m_window->get_loadproc())) {
        throw std::runtime_error("Failed to initialize GLAD");
    }

    m_window->set_framebuffer_size_callback([&] (uint width, uint height) {
        glViewport(0, 0, width, height);
    });

    glEnable(GL_FRAMEBUFFER_SRGB);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.IniFilename = nullptr;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(m_window->get_glfw_window(), true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    m_script_engine->run_script("content/base/scripts/main.lua");

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
    m_window->loop([&] {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGui::Begin("voksel", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Text("hello ><>");
        ImGui::End();

        ImGui::Render();

        process_input();

        glClearColor(powf(0.53, 2.2), powf(0.81, 2.2), powf(0.98, 2.2), 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    });
}

void Client::process_input() {
    if (m_window->is_key_pressed(GLFW_KEY_ESCAPE))
        m_window->close();
}
