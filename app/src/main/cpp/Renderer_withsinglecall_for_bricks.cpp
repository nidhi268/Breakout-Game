#include "Renderer.h"
#include "logging.h"
#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"

#include <cassert>
#include <GLES/egl.h>
#include <GLES3/gl3.h>

#define STB_RECT_PACK_IMPLEMENTATION
#define STB_TRUETYPE_IMPLEMENTATION

#include "stb_rect_pack.h"
#include "stb_truetype.h"

//constexpr auto VERT_CODE = R"(#version 300 es
//precision mediump float;
//
//layout (location = 0) in vec2 a_pos;
//layout (location = 1) in vec2 a_tex_coords;
//
//out vec2 tex_coords;
//
//uniform mat4 projview;
//uniform mat4 model;
//uniform vec2 custom_tex_coords[2];
//
//void main() {
//  gl_Position = projview * model * vec4(a_pos, 0.0, 1.0);
//  tex_coords = a_tex_coords * custom_tex_coords[1] + (1.0 - a_tex_coords) * custom_tex_coords[0];
//}
//)";
//
//
//constexpr auto FRAG_CODE = R"(#version 300 es
//precision mediump float;
//
//in vec2 tex_coords;
//
//uniform sampler2D tex;
//uniform vec4 color;
//
//out vec4 frag_color;
//
//void main() {
//  frag_color = color * texture(tex, tex_coords);
//}
//)";

constexpr auto VERT_CODE = R"(#version 300 es
precision mediump float;

layout (location = 0) in vec2 a_pos;
layout (location = 1) in vec2 a_tex_coords;
layout (location = 2) in vec2 a_instance_pos;
layout (location = 3) in vec4 a_instance_color;

out vec2 tex_coords;
out vec4 v_color;

uniform mat4 projview;
uniform mat4 model;                 // For per-object draw
uniform vec2 custom_tex_coords[2];
uniform vec2 brick_size;           // For instanced draw
uniform bool use_instance;         // Switch mode

void main() {
    vec2 world_pos;
    vec4 color_out;

    if (use_instance) {
        world_pos = a_pos * brick_size + a_instance_pos;
        color_out = a_instance_color;
    } else {
        world_pos = (model * vec4(a_pos, 0.0, 1.0)).xy;
        color_out = vec4(1.0); // fallback white
    }

    gl_Position = projview * vec4(world_pos, 0.0, 1.0);
    tex_coords = a_tex_coords * custom_tex_coords[1] + (1.0 - a_tex_coords) * custom_tex_coords[0];
    v_color = color_out;
}
)";

constexpr auto FRAG_CODE = R"(#version 300 es
precision mediump float;

in vec2 tex_coords;
in vec4 v_color;

uniform sampler2D tex;

out vec4 frag_color;

void main() {
  frag_color = v_color * texture(tex, tex_coords);
}
)";

constexpr uint8_t WHITE[] = {255, 255, 255, 255};

//// (1) Add new VBOs and instance data containers
GLuint instance_vbo_pos = 0;
GLuint instance_vbo_color = 0;
GLsizei brick_instance_count = 0;
float brick_width = 0.f;
float brick_height = 0.f;

std::vector<glm::vec2> brick_positions;
std::vector<glm::vec4> brick_colors;

Renderer::Renderer(android_app *app, Camera &camera) {
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

    glClearColor(0.f, 0.f, 0.f, 1.f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    struct Vertex {
        glm::vec2 position;
        glm::vec2 tex_coords;
    };

    Vertex vertices[] = {
            {{-.5f, .5f},  {0.f, 0.f}},
            {{-.5f, -.5f}, {0.f, 1.f}},
            {{.5f,  -.5f}, {1.f, 1.f}},
            {{.5f,  .5f},  {1.f, 0.f}},
    };

    uint32_t indices[] = {0, 1, 2, 0, 2, 3};

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof indices, indices, GL_STATIC_DRAW);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof vertices, vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void *) offsetof(Vertex, position));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void *) offsetof(Vertex, tex_coords));

    if (!brick_positions.empty()) {
        glGenBuffers(1, &instance_vbo_pos);
        glBindBuffer(GL_ARRAY_BUFFER, instance_vbo_pos);
        glBufferData(GL_ARRAY_BUFFER, brick_positions.size() * sizeof(glm::vec2), brick_positions.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
        glEnableVertexAttribArray(2);
        glVertexAttribDivisor(2, 1);
    }

    if (!brick_colors.empty()) {
        glGenBuffers(1, &instance_vbo_color);
        glBindBuffer(GL_ARRAY_BUFFER, instance_vbo_color);
        glBufferData(GL_ARRAY_BUFFER, brick_colors.size() * sizeof(glm::vec4), brick_colors.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 0, nullptr);
        glEnableVertexAttribArray(3);
        glVertexAttribDivisor(3, 1);
    }

    brick_instance_count = static_cast<GLsizei>(brick_positions.size());


    auto compile_shader = [](GLenum type, const char *src) {
        GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &src, nullptr);
        glCompileShader(shader);

        GLint success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            char info_log[1024];
            glGetShaderInfoLog(shader, 1024, nullptr, info_log);
            LOGE("Failed to compile shader! Info Log: %s", info_log);
        }

        return shader;
    };

    GLuint vert = compile_shader(GL_VERTEX_SHADER, VERT_CODE);
    GLuint frag = compile_shader(GL_FRAGMENT_SHADER, FRAG_CODE);

    program = glCreateProgram();
    glAttachShader(program, vert);
    glAttachShader(program, frag);
    glLinkProgram(program);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char info_log[1024];
        glGetProgramInfoLog(program, 1024, nullptr, info_log);
        LOGE("Failed to link shader! Info Log: %s", info_log);
    }

    glDeleteShader(vert);
    glDeleteShader(frag);

    eglQuerySurface(display, surface, EGL_WIDTH, &width);
    eglQuerySurface(display, surface, EGL_HEIGHT, &height);
    camera.update_projection(static_cast<float>(width), static_cast<float>(height));

    glUseProgram(program);
    glUniform3f(glGetUniformLocation(program, "color"), 1.f, 1.f, 1.f);
    glUniform1i(glGetUniformLocation(program, "tex"), 0);
    glActiveTexture(GL_TEXTURE0);

    projview_location = glGetUniformLocation(program, "projview");
    model_location = glGetUniformLocation(program, "model");
    custom_tex_coords_location = glGetUniformLocation(program, "custom_tex_coords");
    color_location = glGetUniformLocation(program, "color");

    white = std::make_unique<Texture>(1, 1, WHITE);

    auto asset = AAssetManager_open(app->activity->assetManager, "font.ttf", AASSET_MODE_BUFFER);
    if (!asset) {
        LOGE("Failed to load font.ttf");
        return;
    }

    auto buffer = (const uint8_t *) AAsset_getBuffer(asset);

    if (!buffer) {
        LOGE("Failed to load font.ttf");
        return;
    }

    if (!stbtt_InitFont(&font.info, buffer, 0)) {
        LOGE("Failed to initialize font");
        return;
    }

    constexpr auto TEX_SIZE = 1024;
    auto pixels = std::make_unique<uint8_t[]>(TEX_SIZE * TEX_SIZE);

    stbtt_pack_context pack_context;
    if (!stbtt_PackBegin(&pack_context, pixels.get(), TEX_SIZE, TEX_SIZE, 0, 1, nullptr)) {
        LOGE("Failed to start font packing");
        return;
    }

    font.size = 256.f;
    stbtt_pack_range range{
            font.size,
            32,
            nullptr,
            96,
            font.chardata,
    };

    if (!stbtt_PackFontRanges(&pack_context, buffer, 0, &range, 1)) {
        LOGE("Failed to pack font ranges");
        return;
    }

    stbtt_PackEnd(&pack_context);

    auto tex_data = std::make_unique<uint8_t[]>(TEX_SIZE * TEX_SIZE * 4);
    for (size_t i = 0; i < TEX_SIZE * TEX_SIZE; i++) {
        tex_data[4 * i + 0] = 255;
        tex_data[4 * i + 1] = 255;
        tex_data[4 * i + 2] = 255;
        tex_data[4 * i + 3] = pixels[i];
    }

    font.texture = std::make_unique<Texture>(TEX_SIZE, TEX_SIZE, tex_data.get());
    font.loaded = true;
}

Renderer::~Renderer() {
    glDeleteProgram(program);

    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);

    eglDestroyContext(display, context);
    eglDestroySurface(display, surface);
    eglTerminate(display);
}


void Renderer::set_bricks(const std::vector<glm::vec2>& pos,
                          const std::vector<glm::vec4>& col,
                          float width, float height) {
    brick_positions = pos;
    brick_colors = col;
    brick_width = width;
    brick_height = height;
    brick_instance_count = static_cast<GLsizei>(pos.size());

    if (brick_instance_count == 0) return;

    glBindVertexArray(vao);

    // ✅ Generate and bind position VBO
    if (instance_vbo_pos == 0)
        glGenBuffers(1, &instance_vbo_pos);

    glBindBuffer(GL_ARRAY_BUFFER, instance_vbo_pos);
    glBufferData(GL_ARRAY_BUFFER, pos.size() * sizeof(glm::vec2), pos.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(2);
    glVertexAttribDivisor(2, 1);

    // ✅ Generate and bind color VBO
    if (instance_vbo_color == 0)
        glGenBuffers(1, &instance_vbo_color);

    glBindBuffer(GL_ARRAY_BUFFER, instance_vbo_color);
    glBufferData(GL_ARRAY_BUFFER, col.size() * sizeof(glm::vec4), col.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(3);
    glVertexAttribDivisor(3, 1);

    LOGI("set_bricks: uploaded %d instances (vao = %u, posVBO = %u, colorVBO = %u)", brick_instance_count, vao, instance_vbo_pos, instance_vbo_color);
}


void Renderer::do_frame(const RenderData &data) {
    auto draw_char = [&](unsigned char c, float text_size, glm::vec2 position,
                         glm::vec4 color, glm::vec2 &cursor_pos) {
        if (c <= 32 || c > 127) {
            switch (c) {
                case '\n':
                    cursor_pos.y -= text_size * .8f, cursor_pos.x = 0.f;
                    break;
                case ' ':
                    cursor_pos.x += text_size / 3.f;
                    break;
                default:
                    break;
            }

            return;
        }

        auto data = font.chardata[c - 32];

        auto start = glm::vec2{data.xoff, data.yoff} / font.size * text_size;
        auto end = glm::vec2{data.xoff2, data.yoff2} / font.size * text_size;

        auto avg_pos = (start + end) / 2.f;
        avg_pos.y *= -1.f;

        auto T = glm::translate(glm::mat4(1.f), glm::vec3(cursor_pos + position + avg_pos, 0.f));
        auto S = glm::scale(glm::mat4(1.f), glm::vec3(end - start, 1.f));

        auto tex_size = glm::vec2(font.texture->get_width(), font.texture->get_height());
        glm::vec2 tex_coords[2] =
                {glm::vec2{data.x0, data.y0} / tex_size, glm::vec2{data.x1, data.y1} / tex_size};

        glUniform2fv(custom_tex_coords_location, 2, glm::value_ptr(tex_coords[0]));
        glUniformMatrix4fv(model_location, 1, GL_FALSE, glm::value_ptr(T * S));
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

        cursor_pos.x += data.xadvance / font.size * text_size;
    };

    auto get_text_size = [&](float text_size, const std::string &str) {
        glm::vec2 size{0.f, text_size * .8f / 2.f};
        for (auto c : str) {
            if (c <= 32 || c > 127) {
                switch (c) {
                    case '\n':
                        size.y += text_size * .8f;
                        break;
                    case ' ':
                        size.x += text_size / 3.f;
                        break;
                    default:
                        break;
                }
            } else {
                auto data = font.chardata[c - 32];
                size.x += data.xadvance / font.size * text_size;
            }
        }

        return size;
    };

    eglQuerySurface(display, surface, EGL_WIDTH, &width);
    eglQuerySurface(display, surface, EGL_HEIGHT, &height);

    glViewport(0, 0, width, height);
    glClearColor(data.background_color.r, data.background_color.g, data.background_color.b, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);

    data.camera.update_projection(static_cast<float>(width), static_cast<float>(height));
    data.camera.update_view();

    auto projview = data.camera.get_projection() * data.camera.get_view();
    glUniformMatrix4fv(projview_location, 1, GL_FALSE, glm::value_ptr(projview));

    if (font.loaded) {
        glBindTexture(GL_TEXTURE_2D, font.texture->get_id());
        for (const auto &text : data.text_cmds) {
            auto size = get_text_size(text.size, text.str);
            glm::vec2 cursor_pos{};

            Align aligns[] = {text.align_x, text.align_y};
            for (int i = 0; i < 2; i++) {
                auto align = aligns[i];
                float s = size[i], &p = cursor_pos[i];

                switch (align) {
                    case Align::Left:
                        p = 0.f;
                        break;
                    case Align::Center:
                        p = -s / 2.f;
                        break;
                    case Align::Right:
                        p = -s;
                        break;
                }
            }

            for (auto c : text.str) {
                draw_char(c, text.size, text.position, text.color, cursor_pos);
            }
        }
    }


    glm::vec2 default_tex_coords[2] = {{0.f, 0.f}, {1.f, 1.f}};
    glUniform2fv(custom_tex_coords_location, 2, glm::value_ptr(default_tex_coords[0]));

//    if (brick_instance_count > 0) {
//        glBindTexture(GL_TEXTURE_2D, white->get_id());
//        glm::vec2 default_tex_coords[2] = {{0.f, 0.f}, {1.f, 1.f}};
//        glUniform2fv(custom_tex_coords_location, 2, glm::value_ptr(default_tex_coords[0]));
//        glUniform2f(glGetUniformLocation(program, "brick_size"), brick_width * 0.95f, brick_height * 0.95f);
//        glBindVertexArray(vao);
//        glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr, brick_instance_count);
//        glBindVertexArray(vao);
//        glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr, brick_instance_count);
//    }
    if (brick_instance_count > 0) {
        glUseProgram(program); // just to be sure

        glUniform1i(glGetUniformLocation(program, "use_instance"), 1);  // ✅ Tells shader to use instanced path
        glUniform2f(glGetUniformLocation(program, "brick_size"), brick_width * 0.95f, brick_height * 0.95f);

        glm::vec2 default_tex_coords[2] = {{0.f, 0.f}, {1.f, 1.f}};
        glUniform2fv(custom_tex_coords_location, 2, glm::value_ptr(default_tex_coords[0]));

        glBindTexture(GL_TEXTURE_2D, white->get_id());
        glBindVertexArray(vao);

        glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr, brick_instance_count);
    }
    else{
        LOGI("Instanced draw skipped: brick_instance_count is 0");
    }
    // 🧠 Ensure the shader uses per-object drawing for paddle/ball
    glUniform1i(glGetUniformLocation(program, "use_instance"), 0);
    
    for (const auto &cmd : data.draw_cmds) {
        glUniformMatrix4fv(model_location, 1, GL_FALSE, glm::value_ptr(cmd.transformation));
        glUniform4fv(color_location, 1, glm::value_ptr(cmd.color));
        glBindTexture(GL_TEXTURE_2D, cmd.texture ? cmd.texture->get_id() : white->get_id());
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
    }

    auto res = eglSwapBuffers(display, surface);
    assert(res);
}