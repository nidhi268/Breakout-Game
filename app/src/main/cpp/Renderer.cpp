#include "Renderer.h"
#include <GLES/egl.h>
#include <GLES3/gl3.h>
#include <cassert>
#include "logging.h"
#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"

constexpr auto VERT_CODE = R"(#version 300 es
precision mediump float;

layout (location = 0) in vec2 a_pos;

uniform mat4 projection;
uniform mat4 model;

void main() {
    gl_Position = projection * model * vec4(a_pos, 0.0, 1.0);
}
)";

constexpr auto FRAG_CODE = R"(#version 300 es
precision mediump float;

uniform vec3 color;
out vec4 frag_color;

void main() {
    frag_color = vec4(color , 1.0);
}
)";

Renderer::Renderer(android_app *app) {
    display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    assert(display);

    auto res = eglInitialize(display, nullptr, nullptr);
    assert(res == EGL_TRUE);

    {
        EGLint attribs[] = {
                EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
                EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
                EGL_RED_SIZE, 8,
                EGL_BLUE_SIZE, 8,
                EGL_GREEN_SIZE, 8,
                EGL_NONE,
        };
        EGLint num_configs;
        eglChooseConfig(display, attribs, &config, 1, &num_configs);
        assert(num_configs == 1);
    }


    surface = eglCreateWindowSurface(display, config, app->window, nullptr);
    assert(surface != EGL_NO_SURFACE);

    {
        EGLint attribs[] = {
                EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE,
        };

        context = eglCreateContext(display, config, nullptr, attribs);
        assert(context != EGL_NO_CONTEXT);
    }

    res = eglMakeCurrent(display, surface, surface, context);
    assert(res);

    LOGI("EGL initialization complete.");

    glClearColor(1.f,1.f,0.f,1.f);

    float vertices[] = {
            -.5f, .5f,
            -.5f, -.5f,
            .5f, -.5f,
            .5f, .5f,
    };

    uint32_t indices[] = {0,1, 2, 0, 2, 3};

    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof indices, indices, GL_STATIC_DRAW);


    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof vertices, vertices, GL_STATIC_DRAW);

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);

    auto compile_shader = [](GLenum type, const char *src){
        GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &src, nullptr);
        glCompileShader(shader);

        GLint success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if(!success){
            char info_log[1024];
            glGetShaderInfoLog(shader, 1024, nullptr, info_log);
            LOGE("Failed to compile shader! Info Log: %s", info_log);
        }

        return shader;
    } ;

    GLuint vert = compile_shader(GL_VERTEX_SHADER, VERT_CODE);
    GLuint frag = compile_shader(GL_FRAGMENT_SHADER, FRAG_CODE);

    program = glCreateProgram();
    glAttachShader(program, vert);
    glAttachShader(program, frag);
    glLinkProgram(program);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if(!success){
        char info_log[1024];
        glGetProgramInfoLog(program, 1024, nullptr, info_log);
        LOGE("Failed to link shader! Info Log: %s", info_log);
    }

    glDeleteShader(vert);
    glDeleteShader(frag);

    glUseProgram(program);
    glUniform3f(glGetUniformLocation(program, "color"), 1.f, 1.f, 1.f);

    projection_location = glGetUniformLocation(program, "projection");
    model_location = glGetUniformLocation(program, "model");
}

Renderer::~Renderer(){
    glDeleteProgram(program);

    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &vao);

    eglDestroyContext(display, context);
    eglDestroySurface(display, surface);
    eglTerminate(display);
}

void Renderer::do_frame() {
    int width, height;
    eglQuerySurface(display, surface, EGL_WIDTH, &width);
    eglQuerySurface(display, surface, EGL_HEIGHT, &height);

    // where openGl should start drawing from - 0,0 = top left to full width & height
    glViewport(0, 0, width, height);
    // clears screen - to yellow a set in glClearColor
    glClear(GL_COLOR_BUFFER_BIT);

    float inv_aspect = (float)height/(float)width;
    //sets projection matrix type
    glm::mat4 projection = glm::ortho(-1.f, 1.f, -inv_aspect, inv_aspect);

    static float angle = 0.f;
    angle += 0.001f;
    glm::mat4 model{1.f};
    model = glm::rotate(model, angle, {0.f, 0.f, 1.f});

    glUseProgram(program);
    glUniformMatrix4fv(projection_location, 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(model_location, 1, GL_FALSE, glm::value_ptr(model));

    glBindVertexArray(vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

    // updates display
    auto res = eglSwapBuffers(display, surface);
    assert(res);
}
