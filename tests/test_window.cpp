#include <doctest/doctest.h>
#include "test_common.hpp"

#include <exd/app/window.hpp>
#include <exd/app/input_mode.hpp>
#include <exd/app/window_state.hpp>

#include <SDL3/SDL.h>
#include <glad/gl.h>

using namespace exd;
using namespace exd::render;
using namespace exd::render::test;

// ════════════════════════════════════════════════════════════════
// Window loading tests — verifies the exd::app::Window class
// creates a valid SDL window + OpenGL context and can render.
// ════════════════════════════════════════════════════════════════

TEST_SUITE("Window") {

TEST_CASE("Window creates valid SDL window and GL context") {
    app::Window win;

    // After construction, SDL should be initialized and window open
    CHECK(win.native_window() != nullptr);
    CHECK(win.native_context() != nullptr);

    int w, h; float aspect;
    win.get_dimensions(w, h, aspect);

    // Default window is 1280x720
    CHECK(w == 1280);
    CHECK(h == 720);
    CHECK(near(aspect, 1280.0f / 720.0f));

    // GL should be functional
    CHECK(glGetError() == GL_NO_ERROR);
}

TEST_CASE("Window default state values") {
    app::Window win;

    // Default input mode is FPS
    CHECK(win.input_mode == app::InputMode::FPS);

    // Grid should be visible by default
    CHECK(win.grid_visible == true);

    // Wireframe should be off by default
    CHECK(win.wireframe == false);

    // Should not be closing yet
    CHECK(win.should_close() == false);
}

TEST_CASE("Window input mode toggle") {
    app::Window win;

    CHECK(win.input_mode == app::InputMode::FPS);

    win.set_input_mode(app::InputMode::UI);
    CHECK(win.input_mode == app::InputMode::UI);

    win.set_input_mode(app::InputMode::FPS);
    CHECK(win.input_mode == app::InputMode::FPS);
}

TEST_CASE("Window allows GL rendering") {
    app::Window win;

    int w, h; float aspect;
    win.get_dimensions(w, h, aspect);

    glViewport(0, 0, w, h);
    glClearColor(0.5f, 0.0f, 0.5f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    GLenum err = glGetError();
    CHECK(err == GL_NO_ERROR);

    // Read back a pixel to confirm it's the clear color
    unsigned char pixel[4];
    glReadPixels(w / 2, h / 2, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);

    // 0.5 * 255 = 127 (allow ±2 for rounding)
    CHECK(pixel[0] >= 125);
    CHECK(pixel[0] <= 129);
    CHECK(pixel[1] >= 0);
    CHECK(pixel[1] <= 2);
    CHECK(pixel[2] >= 125);
    CHECK(pixel[2] <= 129);
    CHECK(pixel[3] == 255);

    win.swap_buffers();
    CHECK(glGetError() == GL_NO_ERROR);
}

TEST_CASE("Window was_key_released returns false when no events") {
    app::Window win;

    // Without polling events, no key should be reported as released
    CHECK(win.was_key_released(SDL_SCANCODE_ESCAPE) == false);
    CHECK(win.was_key_released(SDL_SCANCODE_SPACE) == false);
    CHECK(win.was_key_released(SDL_SCANCODE_G) == false);
}

TEST_CASE("Window poll_events does not crash") {
    app::Window win;

    // Simply verify polling doesn't crash
    win.poll_events();
    win.poll_events();
    win.poll_events();

    CHECK(glGetError() == GL_NO_ERROR);
}

TEST_CASE("Window dimensions are consistent across multiple calls") {
    app::Window win;

    int w1, h1, w2, h2;
    float a1, a2;

    win.get_dimensions(w1, h1, a1);
    win.get_dimensions(w2, h2, a2);

    CHECK(w1 == w2);
    CHECK(h1 == h2);
    CHECK(near(a1, a2));
}

TEST_CASE("Window wireframe toggle state is writable") {
    app::Window win;

    CHECK(win.wireframe == false);
    win.wireframe = true;
    CHECK(win.wireframe == true);
    win.wireframe = false;
    CHECK(win.wireframe == false);
}

TEST_CASE("Window grid_visible toggle state is writable") {
    app::Window win;

    CHECK(win.grid_visible == true);
    win.grid_visible = false;
    CHECK(win.grid_visible == false);
    win.grid_visible = true;
    CHECK(win.grid_visible == true);
}

TEST_CASE("Window survives multiple swap_buffers cycles") {
    app::Window win;

    for (int i = 0; i < 5; ++i) {
        glClear(GL_COLOR_BUFFER_BIT);
        win.swap_buffers();
        win.poll_events();
    }

    CHECK(glGetError() == GL_NO_ERROR);
}

} // TEST_SUITE("Window")
