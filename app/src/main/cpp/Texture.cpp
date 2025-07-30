#include "Texture.h"

#include <android/asset_manager.h>
#include <GLES3/gl3.h>

#define STB_IMAGE_IMPLEMENTATION

#include "stb_image.h"
#include "logging.h"

Texture::Texture(AAssetManager *asset_manager, const std::string &file_path) {
    auto asset = AAssetManager_open(asset_manager, file_path.c_str(), AASSET_MODE_BUFFER);
    auto length = AAsset_getLength(asset);
    auto buffer = AAsset_getBuffer(asset);

    if (!buffer) {
        LOGE("Failed to get asset buffer");
        AAsset_close(asset);
        return;
    }

    auto data = stbi_load_from_memory((const uint8_t *) buffer, length, &width, &height, nullptr, 4);
    AAsset_close(asset);

    if (!data) {
        LOGE("Failed to load image: %s", stbi_failure_reason());
        return;
    }

    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, width, height);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);

    stbi_image_free(data);
}

Texture::Texture(int width, int height, const uint8_t *data) : width(width), height(height) {
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, width, height);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);
}

Texture::~Texture() {
    glDeleteTextures(1, &id);
}
