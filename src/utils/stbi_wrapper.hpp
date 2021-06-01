#pragma once

#include <optional>
#include <cstdlib>
#include <utility>

namespace osc::stbi {

    void image_free(void*);
    char const* failure_reason();
    void set_flip_vertically_on_load(bool);

    struct Image final {
        int width;
        int height;
        int channels;
        unsigned char* data;

        // if empty, use `failure_reason` to retrieve the reason why
        static std::optional<Image> load(char const* path);

    private:
        // used internally by load(path)
        Image(int w, int h, int c, unsigned char* ptr) :
            width{w}, height{h}, channels{c}, data{ptr} {
        }

    public:
        Image(Image const&) = delete;

        Image(Image&& tmp) noexcept :
            width{tmp.width},
            height{tmp.height},
            channels{tmp.channels},
            data{tmp.data} {

            tmp.data = nullptr;
        }

        ~Image() noexcept {
            if (data) {
                image_free(data);
            }
        }

        Image& operator=(Image const&) = delete;

        Image& operator=(Image&& tmp) noexcept {
            width = tmp.width;
            height = tmp.height;
            channels = tmp.channels;
            std::swap(data, tmp.data);

            return *this;
        }
    };
}
