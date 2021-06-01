#include "stbi_wrapper.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

void osc::stbi::image_free(void* p) {
    stbi_image_free(p);
}

char const* osc::stbi::failure_reason() {
    return stbi_failure_reason();
}

void osc::stbi::set_flip_vertically_on_load(bool v) {
    stbi_set_flip_vertically_on_load(v);
}

std::optional<osc::stbi::Image> osc::stbi::Image::load(char const* path) {
    int width;
    int height;
    int channels;
    unsigned char* data = stbi_load(path, &width, &height, &channels, 0);

    if (data) {
        return Image{width, height, channels, data};
    } else {
        return std::nullopt;
    }
}
