#pragma once

#include <memory>
#include <GLES/egl.h>
#include <game-activity/native_app_glue/android_native_app_glue.h>

#include "Texture.h"
#include "glm/glm.hpp"
#include "stb_truetype.h"

struct DrawCommand {
    glm::mat4 transformation;
    Texture *texture;
};

enum class Align {
    Left,
    Center,
    Right,
    Top = Right,
    Bottom = Left,
};

struct TextCommand {
    std::string str{};
    glm::vec2 position{};
    float size = 1.f;
    glm::vec4 color{1.f};

    Align align_x = Align::Center, align_y = Align::Center;
};

class Renderer {
public:
    explicit Renderer(android_app *app);
    ~Renderer();

    void do_frame(const std::vector<DrawCommand> &cmds, const std::vector<TextCommand> &text_cmds);

    glm::ivec2 get_size() const {return {width, height};}
    glm::mat4 get_projection() const {return projection;}

private:
    EGLDisplay display;
    EGLConfig config;
    EGLSurface surface;
    EGLContext context;

    GLuint vao{}, vbo{}, ebo{};
    GLuint program;
    GLint projection_location, model_location, custom_tex_coords_location;

    int width, height;
    glm::mat4 projection;
    std::unique_ptr<Texture> white;

    struct Font {
        bool loaded{};
        float size;
        std::unique_ptr<Texture> texture;

        stbtt_fontinfo info;
        stbtt_packedchar chardata[96];
    } font{};
};