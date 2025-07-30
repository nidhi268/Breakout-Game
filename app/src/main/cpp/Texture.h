//
// Created by nidshu01 on 7/30/25.
//

#ifndef BREAKOUTGAME_TEXTURE_H
#define BREAKOUTGAME_TEXTURE_H


#pragma once

#include <string>
#include <GLES3/gl3.h>
#include <android/asset_manager.h>

class Texture {
public:
    Texture(AAssetManager *asset_manager, const std::string &file_path);
    Texture(int width, int height, const uint8_t *data);
    ~Texture();

    [[nodiscard]] auto get_width() const { return width; }

    [[nodiscard]] auto get_height() const { return height; }

    [[nodiscard]] auto get_id() const { return id; }

private:
    int width{}, height{};
    GLuint id{};
};

#endif //BREAKOUTGAME_TEXTURE_H
