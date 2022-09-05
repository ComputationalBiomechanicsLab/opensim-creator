#pragma once

#include "src/Graphics/Image.hpp"
#include "src/Maths/Rect.hpp"

#include <string>
#include <vector>

namespace osc
{
    struct AnnotatedImage final {

        struct Annotation final {
            std::string label;
            Rect rect;
        };

        Image image;
        std::vector<Annotation> annotations;
    };
}