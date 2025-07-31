#pragma once

#include <memory>
#include <GLES/egl.h>
#include <game-activity/native_app_glue/android_native_app_glue.h>

#include "Texture.h"
#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"

struct DrawCommand {
    glm::mat4 transformation;
    Texture *texture;
};

class Renderer {
public:
    explicit Renderer(android_app *app);
    ~Renderer();

    void do_frame(const std::vector<DrawCommand> &cmds);

private:
    EGLDisplay display;
    EGLConfig config;
    EGLSurface surface;
    EGLContext context;

    GLuint vao{}, vbo{}, ebo{};
    GLuint program;
    GLint projection_location, model_location;

    std::unique_ptr<Texture> white;
};