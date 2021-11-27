#pragma once

#include <assert.h>
#include <chrono>
#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <windows.h>

#include "graphics/graphics.hpp"
#include "platform.hpp"
#include "fracas_client.hpp"

const static int FRAME_RATE_HZ = 60;
const static long long FRAME_TIME_NS = 1000000000 / FRAME_RATE_HZ;

uint32_t OUTPUT_BUFFER_WIDTH = 1920;
uint32_t OUTPUT_BUFFER_HEIGHT = 1080;

GLFWwindow *window;

void set_fullscreen(bool enable)
{
    if (enable)
    {
        GLFWmonitor *monitor = glfwGetPrimaryMonitor();
        glfwSetWindowMonitor(window, monitor, 0, 0, OUTPUT_BUFFER_WIDTH, OUTPUT_BUFFER_HEIGHT, GLFW_DONT_CARE);
    }
    else
    {
        glfwSetWindowMonitor(window, NULL, 50, 50, OUTPUT_BUFFER_WIDTH, OUTPUT_BUFFER_HEIGHT, GLFW_DONT_CARE);
    }
}

struct GlfwState
{
    InputState *input_state;
    RenderTarget *main_target;
};

void fill_input_state(GLFWwindow *window, InputState *state)
{
    // reset per frame data
    state->text_input = {};
    state->key_input = {};
    state->mouse_down_event = false;
    state->mouse_up_event = false;
    state->scrollwheel_count = 0;

    double mouse_x;
    double mouse_y;
    state->prev_mouse_x = state->mouse_x;
    state->prev_mouse_y = state->mouse_y;
    glfwGetCursorPos(window, &mouse_x, &mouse_y);
    state->mouse_x = mouse_x;
    state->mouse_y = mouse_y;

    for (int i = 0; i < (int)Keys::INVALID; i++)
    {
        state->key_down_events[i] = false;
        state->key_up_events[i] = false;
    }
}

void character_input_callback(GLFWwindow *window, unsigned int codepoint)
{
    InputState *input_state = static_cast<GlfwState *>(glfwGetWindowUserPointer(window))->input_state;

    if (codepoint < 256) // only doing ASCII
    {
        input_state->text_input.append(codepoint);
    }
}

void key_input_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    InputState *input_state = static_cast<GlfwState *>(glfwGetWindowUserPointer(window))->input_state;

    auto append_if_press = [&](Keys k)
    {
        input_state->keys[(int)k] = (action == GLFW_PRESS || action == GLFW_REPEAT);
        if (action == GLFW_PRESS || action == GLFW_REPEAT)
        {
            input_state->key_input.append(k);
            input_state->key_down_events[(int)k] = true;
        }
        else
        {
            input_state->key_up_events[(int)k] = true;
        }
    };

    switch (key)
    {
    case GLFW_KEY_0:
    case GLFW_KEY_KP_0:
        append_if_press(Keys::NUM_0);
        break;
    case GLFW_KEY_1:
    case GLFW_KEY_KP_1:
        append_if_press(Keys::NUM_1);
        break;
    case GLFW_KEY_2:
    case GLFW_KEY_KP_2:
        append_if_press(Keys::NUM_2);
        break;
    case GLFW_KEY_3:
    case GLFW_KEY_KP_3:
        append_if_press(Keys::NUM_3);
        break;
    case GLFW_KEY_4:
    case GLFW_KEY_KP_4:
        append_if_press(Keys::NUM_4);
        break;
    case GLFW_KEY_5:
    case GLFW_KEY_KP_5:
        append_if_press(Keys::NUM_5);
        break;
    case GLFW_KEY_6:
    case GLFW_KEY_KP_6:
        append_if_press(Keys::NUM_6);
        break;
    case GLFW_KEY_7:
    case GLFW_KEY_KP_7:
        append_if_press(Keys::NUM_7);
        break;
    case GLFW_KEY_8:
    case GLFW_KEY_KP_8:
        append_if_press(Keys::NUM_8);
        break;
    case GLFW_KEY_9:
    case GLFW_KEY_KP_9:
        append_if_press(Keys::NUM_9);
        break;
    case GLFW_KEY_SPACE:
        append_if_press(Keys::SPACE);
        break;
    case GLFW_KEY_BACKSPACE:
        append_if_press(Keys::BACKSPACE);
        break;
    case GLFW_KEY_ENTER:
        append_if_press(Keys::ENTER);
        break;
    case GLFW_KEY_W:
        append_if_press(Keys::W);
        break;
    case GLFW_KEY_A:
        append_if_press(Keys::A);
        break;
    case GLFW_KEY_S:
        append_if_press(Keys::S);
        break;
    case GLFW_KEY_D:
        append_if_press(Keys::D);
        break;
    case GLFW_KEY_Z:
        append_if_press(Keys::Z);
        break;
    case GLFW_KEY_X:
        append_if_press(Keys::X);
        break;
    case GLFW_KEY_C:
        append_if_press(Keys::C);
        break;
    case GLFW_KEY_UP:
        append_if_press(Keys::UP);
        break;
    case GLFW_KEY_DOWN:
        append_if_press(Keys::DOWN);
        break;
    case GLFW_KEY_LEFT:
        append_if_press(Keys::LEFT);
        break;
    case GLFW_KEY_RIGHT:
        append_if_press(Keys::RIGHT);
        break;
    case GLFW_KEY_F1:
        append_if_press(Keys::F1);
        break;
    case GLFW_KEY_F2:
        append_if_press(Keys::F2);
        break;
    case GLFW_KEY_F3:
        append_if_press(Keys::F3);
        break;
    case GLFW_KEY_F4:
        append_if_press(Keys::F4);
        break;
    case GLFW_KEY_F5:
        append_if_press(Keys::F5);
        break;
    case GLFW_KEY_F6:
        append_if_press(Keys::F6);
        break;
    case GLFW_KEY_F7:
        append_if_press(Keys::F7);
        break;
    case GLFW_KEY_F8:
        append_if_press(Keys::F8);
        break;
    case GLFW_KEY_F9:
        append_if_press(Keys::F9);
        break;
    case GLFW_KEY_F10:
        append_if_press(Keys::F10);
        break;
    case GLFW_KEY_F11:
        append_if_press(Keys::F11);
        break;
    case GLFW_KEY_F12:
        append_if_press(Keys::F12);
        break;
    case GLFW_KEY_LEFT_CONTROL:
        append_if_press(Keys::LCTRL);
        break;
    case GLFW_KEY_RIGHT_CONTROL:
        append_if_press(Keys::RCTRL);
        break;
    case GLFW_KEY_LEFT_ALT:
        append_if_press(Keys::LALT);
        break;
    case GLFW_KEY_RIGHT_ALT:
        append_if_press(Keys::RALT);
        break;
    case GLFW_KEY_ESCAPE:
        append_if_press(Keys::ESCAPE);
        break;
    }
}

void mouse_button_callback(GLFWwindow *window, int button, int action, int mods)
{
    InputState *input_state = static_cast<GlfwState *>(glfwGetWindowUserPointer(window))->input_state;

    switch (button)
    {
    case (GLFW_MOUSE_BUTTON_LEFT):
    {
        input_state->mouse_left = action == GLFW_PRESS;

        if (action == GLFW_PRESS)
            input_state->mouse_down_event = true;
        else
            input_state->mouse_up_event = true;
    }
    break;
    }
}

void scroll_callback(GLFWwindow *window, double x_offset, double y_offset)
{
    InputState *input_state = static_cast<GlfwState *>(glfwGetWindowUserPointer(window))->input_state;

    input_state->scrollwheel_count += y_offset;
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    RenderTarget *target = static_cast<GlfwState *>(glfwGetWindowUserPointer(window))->main_target;
    target->width = width;
    target->height = height;
}

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwSwapInterval(1); // vsync on
    glfwWindowHint(GLFW_SAMPLES, 4);

#if MACOS
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    window = glfwCreateWindow(OUTPUT_BUFFER_WIDTH, OUTPUT_BUFFER_HEIGHT, "El Dorado", NULL, NULL);
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

    InputState input_state = {};
    RenderTarget target = init_graphics(OUTPUT_BUFFER_WIDTH, OUTPUT_BUFFER_HEIGHT);

    GlfwState glfw_state = {&input_state, &target};
    glfwSetWindowUserPointer(window, &glfw_state);
    glfwSetCharCallback(window, character_input_callback);
    glfwSetKeyCallback(window, key_input_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    auto loop_start_time = std::chrono::high_resolution_clock::now();
    while (!glfwWindowShouldClose(window))
    {
        //printf("loop time: %lldms\n", (std::chrono::high_resolution_clock::now() - loop_start_time).count() / 1000000);
        loop_start_time = std::chrono::high_resolution_clock::now();

        target.clear();

        fill_input_state(window, &input_state);
        glfwPollEvents();

        if (!game_update(1.f / FRAME_RATE_HZ, &input_state, target))
            glfwSetWindowShouldClose(window, true);

        glfwSwapBuffers(window);
    }

    glfwTerminate();

    exit(0);
    return 0;
}

FileData read_entire_file(const char *filename)
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
    if (file_handle == INVALID_HANDLE_VALUE)
    {
        return {nullptr, 0};
    }

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

    CloseHandle(file_handle);

    return res;
}

// BIG TODO: actually use this
void free_file(FileData file)
{
    free(file.data);
}

FileData read_entire_file(const char *filename, StackAllocator *allocator)
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
    if (file_handle == INVALID_HANDLE_VALUE)
    {
        return {nullptr, 0};
    }

    LARGE_INTEGER filesize;
    GetFileSizeEx(file_handle, &filesize);

    res.length = filesize.QuadPart;
    res.data = allocator->alloc(res.length + 1);
    res.data[res.length] = '\0';

    DWORD read;
    ReadFile(
        file_handle,
        res.data,
        res.length,
        &read,
        NULL);

    CloseHandle(file_handle);

    return res;
}

void write_file(const char *filename, String data)
{
    auto file_handle = CreateFileA(
        filename,
        GENERIC_WRITE,
        NULL,
        NULL,
        CREATE_ALWAYS,
        NULL,
        NULL);
    if (file_handle == INVALID_HANDLE_VALUE)
    {
        printf("ERROR writing file: %s. Dumping to stdout\n", filename);
        printf("%.*s\n", data.len, data.data);
        return;
    }

    DWORD written = 0;
    if (!WriteFile(file_handle, data.data, data.len, &written, nullptr) || written != data.len)
    {
        printf("ERROR writing file: %s. Dumping to stdout\n", filename);
        printf("%.*s\n", data.len, data.data);
    }

    CloseHandle(file_handle);
}

uint64_t debug_get_cycle_count()
{
    return __rdtsc();
}