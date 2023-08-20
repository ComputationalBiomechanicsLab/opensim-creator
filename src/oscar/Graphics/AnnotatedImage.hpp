#pragma once

#include "oscar/Graphics/ImageAnnotation.hpp"
#include "oscar/Graphics/Texture2D.hpp"

#include <vector>

namespace osc
{
    struct AnnotatedImage final {
        Texture2D image;
        std::vector<ImageAnnotation> annotations;
    };
}
