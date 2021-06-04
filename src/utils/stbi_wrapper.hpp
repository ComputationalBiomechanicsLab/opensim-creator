#pragma once

#include <optional>

// stbi: basic C++ wrappers for the STBI API
//
// this is here so that the rest of the codebase doesn't have to
// #include STBI headers, which can be huge (e.g. stb_image.h is
// >7 kLOC)

namespace osc::stbi {

    // free image data loaded by STBI
    void image_free(void*);

    // get a string representing the last failure that occurred in STBI
    char const* failure_reason();

    // globally set whether loaded images should be vertically flipped
    void set_flip_vertically_on_load(bool);

    // an image loaded via STBI
    struct Image final {
        int width;
        int height;

        // number of color channels in the image
        //
        // assume one byte per channel
        int channels;

        // raw data, containing intercalated color channels, e.g.:
        //
        // [c0, c1, c2, c3, c0, c1, c2, c3]
        //
        // or, more directly:
        //
        // [R, G, B, A, R, G, B, A]
        //
        // although it's more "correct" better to think of it in terms of
        // channels, because some images aren't color (e.g. greyscale,
        // heightmaps, normal maps)
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

        constexpr Image(Image&& tmp) noexcept :
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

        constexpr Image& operator=(Image&& tmp) noexcept {
            width = tmp.width;
            height = tmp.height;
            channels = tmp.channels;

            unsigned char* ptr = data;
            data = tmp.data;
            tmp.data = ptr;

            return *this;
        }
    };
}
