#include "src/Graphics/Renderer.hpp"
#include "src/Platform/App.hpp"

#include <gtest/gtest.h>


static constexpr char const g_ShaderSrc[] =
R"(
    BEGIN_VERTEX_SHADER

    #version 330 core

    in vec3 aPos;

    void main()
    {
        gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
    }

    END_VERTEX_SHADER

    BEGIN_FRAGMENT_SHADER

    #version 330 core

    out vec4 FragColor;
    uniform vec4 uColor;

    void main()
    {
        FragColor = uColor;
    }

    END_FRAGMENT_SHADER
)";

TEST(Shader, CanCompileBasicSource)
{
    osc::App app;
    // osc::experimental::Shader s{g_ShaderSrc}; TODO
}