#pragma once

#include <assert.h>
#include <chrono>
#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <windows.h>

#include "fracas_client.hpp"
#include "graphics.hpp"
#include "platform.hpp"

const static int FRAME_RATE_HZ = 60;
const static long long FRAME_TIME_NS = 1000000000 / FRAME_RATE_HZ;

uint32_t OUTPUT_BUFFER_WIDTH = 1920;
uint32_t OUTPUT_BUFFER_HEIGHT = 1080;

void fill_input_state(GLFWwindow *window, InputState *state)
{
    *state = {};

    glfwGetCursorPos(window, &state->mouse_x, &state->mouse_y);
}

void character_input_callback(GLFWwindow *window, unsigned int codepoint)
{
    InputState *input_state = static_cast<InputState *>(glfwGetWindowUserPointer(window));

    if (codepoint < 256) // only doing ASCII
    {
        input_state->text_input.append(codepoint);
    }
}

void key_input_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    InputState *input_state = static_cast<InputState *>(glfwGetWindowUserPointer(window));

    if (action == GLFW_PRESS || action == GLFW_REPEAT)
    {
        switch (key)
        {
        case GLFW_KEY_BACKSPACE:
            input_state->key_input.append(Keys::BACKSPACE);
            break;
        case GLFW_KEY_A:
            input_state->key_input.append(Keys::A);
            break;
        case GLFW_KEY_S:
            input_state->key_input.append(Keys::S);
            break;
        case GLFW_KEY_D:
            input_state->key_input.append(Keys::D);
            break;
        }
    }
}

void mouse_button_callback(GLFWwindow *window, int button, int action, int mods)
{
    InputState *input_state = static_cast<InputState *>(glfwGetWindowUserPointer(window));

    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        input_state->mouse_input.append({action == GLFW_PRESS});
    }
}

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwSwapInterval(1); // vsync on

#if MACOS
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow *window = glfwCreateWindow(OUTPUT_BUFFER_WIDTH, OUTPUT_BUFFER_HEIGHT, "El Dorado", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    init_graphics();

    InputState input_state = {};
    glfwSetWindowUserPointer(window, &input_state);
    glfwSetCharCallback(window, character_input_callback);
    glfwSetKeyCallback(window, key_input_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    RenderTarget render_target = {OUTPUT_BUFFER_WIDTH, OUTPUT_BUFFER_HEIGHT};

    auto loop_start_time = std::chrono::high_resolution_clock::now();
    while (!glfwWindowShouldClose(window))
    {
        //printf("loop time: %lldms\n", (std::chrono::high_resolution_clock::now() - loop_start_time).count() / 1000000);
        loop_start_time = std::chrono::high_resolution_clock::now();

        clear_backbuffer();

        fill_input_state(window, &input_state);
        glfwPollEvents();

        if (!game_update(1.f / FRAME_RATE_HZ, &input_state, render_target))
            glfwSetWindowShouldClose(window, true);

        glfwSwapBuffers(window);
    }

    glfwTerminate();

    exit(0);
    return 0;
}

FileData
read_entire_file(const char *filename)
{
    FileData res;

    auto file_handle = CreateFileA(
        filename,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        NULL,
        NULL);

    LARGE_INTEGER filesize;
    GetFileSizeEx(file_handle, &filesize);

    res.length = filesize.QuadPart;
    res.data = (char *)malloc(res.length + 1);
    res.data[res.length] = '\0';

    DWORD read;
    ReadFile(
        file_handle,
        res.data,
        res.length,
        &read,
        NULL);

    return res;
}

uint64_t debug_get_cycle_count()
{
    return __rdtsc();
}