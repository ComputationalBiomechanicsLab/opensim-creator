#pragma once

#include "oscar/Graphics/Image.hpp"
#include "oscar/Graphics/ImageAnnotation.hpp"

#include <vector>

namespace osc
{
    struct AnnotatedImage final {
        Image image;
        std::vector<ImageAnnotation> annotations;
    };
}