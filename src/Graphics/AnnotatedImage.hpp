#pragma once

#include "src/Graphics/Image.hpp"
#include "src/Graphics/ImageAnnotation.hpp"

#include <vector>

namespace osc
{
    struct AnnotatedImage final {
        Image image;
        std::vector<ImageAnnotation> annotations;
    };
}