#pragma once

#include <memory>
#include <GLES/egl.h>
#include <game-activity/native_app_glue/android_native_app_glue.h>

#include "Texture.h"

class Renderer {
public:
    Renderer(android_app *app);
    ~Renderer();

    void do_frame();

private:
    EGLDisplay display;
    EGLConfig config;
    EGLSurface surface;
    EGLContext context;

    GLuint vao{}, vbo{}, ebo{};
    GLuint program;
    GLint projection_location, model_location;

    std::unique_ptr<Texture> texture;
};