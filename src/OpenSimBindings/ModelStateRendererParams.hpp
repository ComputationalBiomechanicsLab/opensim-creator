#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <nonstd/span.hpp>

namespace gl
{
    class Texture2D;
    class FrameBuffer;
}

namespace osc
{
    struct ComponentDecoration;
}

namespace osc
{
    class ModelStateRendererParams final {
    public:
        glm::ivec2 Dimensions;
        int Samples;
        bool WireframeMode;
        bool DrawMeshNormals;
        bool DrawRims;
        bool DrawFloor;
        glm::mat4 ViewMatrix;
        glm::mat4 ProjectionMatrix;
        glm::vec3 ViewPos;
        glm::vec3 LightDirection;
        glm::vec3 LightColor;
        glm::vec4 BackgroundColor;
        glm::vec4 RimColor;
        glm::vec3 FloorLocation;
        float FixupScaleFactor;

        ModelStateRendererParams();
    };

    bool operator==(ModelStateRendererParams const&, ModelStateRendererParams const&);
    bool operator!=(ModelStateRendererParams const&, ModelStateRendererParams const&);
}