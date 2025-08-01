#pragma once

#include <memory>
#include <GLES/egl.h>
#include <game-activity/native_app_glue/android_native_app_glue.h>
#include <vector>
#include <functional>
#include "glm/glm.hpp"
#include "Texture.h"
#include "stb_truetype.h"
#include "Camera.h"

struct DrawCommand {
    glm::mat4 transformation;
    Texture *texture;
    glm::vec4 color;
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

struct RenderData {
    Camera &camera;
    glm::vec3 background_color;
    std::vector<DrawCommand> draw_cmds;
    std::vector<TextCommand> text_cmds;
};

class Renderer {
public:
    explicit Renderer(android_app *app, Camera &camera);
    ~Renderer();

    void do_frame(const RenderData &data);

    glm::ivec2 get_size() const { return {width, height}; }
    void set_bricks(const std::vector<glm::vec2>& pos,
                    const std::vector<glm::vec4>& col,
                    float width, float height);

private:
    EGLDisplay display{};
    EGLConfig config{};
    EGLSurface surface{};
    EGLContext context{};

    GLuint vao{}, vbo{}, ebo{};
    GLuint program;
    GLint projview_location, model_location, custom_tex_coords_location, color_location;

    int width{}, height{};
    std::unique_ptr<Texture> white;

    struct Font {
        bool loaded{};
        float size;
        std::unique_ptr<Texture> texture;

        stbtt_fontinfo info;
        stbtt_packedchar chardata[96];
    } font{};
};