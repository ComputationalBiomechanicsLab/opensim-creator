#include <oscar/Graphics/Shader.h>

#include <testoscar/testoscarconfig.h>

#include <gtest/gtest.h>
#include <oscar/Platform/App.h>
#include <oscar/Platform/AppMetadata.h>
#include <oscar/Utils/Algorithms.h>
#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/StringHelpers.h>

#include <array>
#include <cstddef>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_set>
#include <utility>

using namespace osc;

namespace
{
    std::unique_ptr<App> g_shader_app;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

    class ShaderTest : public ::testing::Test {
    protected:
        static void SetUpTestSuite()
        {
            const AppMetadata metadata{TESTOSCAR_ORGNAME_STRING, TESTOSCAR_APPNAME_STRING};
            g_shader_app = std::make_unique<App>(metadata);
        }

        static void TearDownTestSuite()
        {
            g_shader_app.reset();
        }
    };

    constexpr CStringView c_vertex_shader_src = R"(
        #version 330 core

        uniform mat4 uViewProjMat;
        uniform mat4 uLightSpaceMat;
        uniform vec3 uLightDir;
        uniform vec3 uViewPos;
        uniform float uDiffuseStrength = 0.85f;
        uniform float uSpecularStrength = 0.4f;
        uniform float uShininess = 8;

        layout (location = 0) in vec3 aPos;
        layout (location = 2) in vec3 aNormal;
        layout (location = 6) in mat4 aModelMat;
        layout (location = 10) in mat3 aNormalMat;

        out vec3 FragWorldPos;
        out vec4 FragLightSpacePos;
        out vec3 NormalWorldDir;
        out float NonAmbientBrightness;

        void main()
        {
            vec3 normalDir = normalize(aNormalMat * aNormal);
            vec3 fragPos = vec3(aModelMat * vec4(aPos, 1.0));
            vec3 frag2viewDir = normalize(uViewPos - fragPos);
            vec3 frag2lightDir = normalize(-uLightDir);  // light direction is in the opposite direction
            vec3 halfwayDir = 0.5 * (frag2lightDir + frag2viewDir);

            float diffuseAmt = uDiffuseStrength * abs(dot(normalDir, frag2lightDir));
            float specularAmt = uSpecularStrength * pow(abs(dot(normalDir, halfwayDir)), uShininess);

            vec4 worldPos = aModelMat * vec4(aPos, 1.0);

            FragWorldPos = vec3(aModelMat * vec4(aPos, 1.0));
            FragLightSpacePos = uLightSpaceMat * worldPos;
            NormalWorldDir = normalDir;
            NonAmbientBrightness = diffuseAmt + specularAmt;

            gl_Position = uViewProjMat * worldPos;
        }
    )";

    constexpr CStringView c_fragment_shader_src = R"(
        #version 330 core

        uniform bool uHasShadowMap = false;
        uniform vec3 uLightDir;
        uniform sampler2D uShadowMapTexture;
        uniform float uAmbientStrength = 0.15f;
        uniform vec3 uLightColor;
        uniform vec4 uDiffuseColor = vec4(1.0, 1.0, 1.0, 1.0);
        uniform float uNear;
        uniform float uFar;

        in vec3 FragWorldPos;
        in vec4 FragLightSpacePos;
        in vec3 NormalWorldDir;
        in float NonAmbientBrightness;

        out vec4 Color0Out;

        float CalculateShadowAmount()
        {
            // perspective divide
            vec3 projCoords = FragLightSpacePos.xyz / FragLightSpacePos.w;

            // map to [0, 1]
            projCoords = 0.5*projCoords + 0.5;

            // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
            float closestDepth = texture(uShadowMapTexture, projCoords.xy).r;

            // get depth of current fragment from light's perspective
            float currentDepth = projCoords.z;

            // calculate bias (based on depth map resolution and slope)
            float bias = max(0.025 * (1.0 - abs(dot(NormalWorldDir, uLightDir))), 0.0025);

            // check whether current frag pos is in shadow
            // float shadow = currentDepth - bias > closestDepth  ? 1.0 : 0.0;
            // PCF
            float shadow = 0.0;
            vec2 texelSize = 1.0 / textureSize(uShadowMapTexture, 0);
            for(int x = -1; x <= 1; ++x)
            {
                for(int y = -1; y <= 1; ++y)
                {
                    float pcfDepth = texture(uShadowMapTexture, projCoords.xy + vec2(x, y) * texelSize).r;
                    if (pcfDepth < 1.0)
                    {
                        shadow += (currentDepth - bias) > pcfDepth  ? 1.0 : 0.0;
                    }
                }
            }
            shadow /= 9.0;

            return shadow;
        }

        float LinearizeDepth(float depth)
        {
            // from: https://learnopengl.com/Advanced-OpenGL/Depth-testing
            //
            // only really works with perspective cameras: orthogonal cameras
            // don't need this unprojection math trick

            float z = depth * 2.0 - 1.0;
            return (2.0 * uNear * uFar) / (uFar + uNear - z * (uFar - uNear));
        }

        void main()
        {
            float shadowAmt = uHasShadowMap ? 0.5*CalculateShadowAmount() : 0.0f;
            float brightness = uAmbientStrength + ((1.0 - shadowAmt) * NonAmbientBrightness);
            Color0Out = vec4(brightness * uLightColor, 1.0) * uDiffuseColor;
            Color0Out.a *= 1.0 - (LinearizeDepth(gl_FragCoord.z) / uFar);  // fade into background at high distances
            Color0Out.a = clamp(Color0Out.a, 0.0, 1.0);
        }
    )";

    // expected, based on the above shader code
    constexpr auto c_expected_property_names = std::to_array<CStringView>({
        "uViewProjMat",
        "uLightSpaceMat",
        "uLightDir",
        "uViewPos",
        "uDiffuseStrength",
        "uSpecularStrength",
        "uShininess",
        "uHasShadowMap",
        "uShadowMapTexture",
        "uAmbientStrength",
        "uLightColor",
        "uDiffuseColor",
        "uNear",
        "uFar",
    });

    constexpr std::array<ShaderPropertyType, 14> c_expected_property_types = std::to_array(
    {
        ShaderPropertyType::Mat4,
        ShaderPropertyType::Mat4,
        ShaderPropertyType::Vec3,
        ShaderPropertyType::Vec3,
        ShaderPropertyType::Float,
        ShaderPropertyType::Float,
        ShaderPropertyType::Float,
        ShaderPropertyType::Bool,
        ShaderPropertyType::Sampler2D,
        ShaderPropertyType::Float,
        ShaderPropertyType::Vec3,
        ShaderPropertyType::Vec4,
        ShaderPropertyType::Float,
        ShaderPropertyType::Float,
    });
    static_assert(c_expected_property_names.size() == c_expected_property_types.size());

    constexpr CStringView c_geometry_shader_vert_src = R"(
        #version 330 core

        // draw_normals: program that draws mesh normals
        //
        // This vertex shader just passes each vertex/normal to the geometry shader, which
        // then uses that information to draw lines for each normal.

        layout (location = 0) in vec3 aPos;
        layout (location = 2) in vec3 aNormal;

        out VS_OUT {
            vec3 normal;
        } vs_out;

        void main()
        {
            gl_Position = vec4(aPos, 1.0f);
            vs_out.normal = aNormal;
        }
    )";

    constexpr CStringView c_geometry_shader_geom_src = R"(
        #version 330 core

        // draw_normals: program that draws mesh normals
        //
        // This geometry shader generates a line strip for each normal it is given. The downstream
        // fragment shader then fills in each line, so that the viewer can see normals as lines
        // poking out of the mesh

        uniform mat4 uModelMat;
        uniform mat4 uViewProjMat;
        uniform mat4 uNormalMat;

        layout (triangles) in;
        in VS_OUT {
            vec3 normal;
        } gs_in[];

        layout (line_strip, max_vertices = 6) out;

        const float NORMAL_LINE_LEN = 0.01f;

        void GenerateLine(int index)
        {
            vec4 origVertexPos = uViewProjMat * uModelMat * gl_in[index].gl_Position;

            // emit original vertex in original position
            gl_Position = origVertexPos;
            EmitVertex();

            // calculate normal vector *direction*
            vec4 normalVec = normalize(uViewProjMat * uNormalMat * vec4(gs_in[index].normal, 0.0f));

            // then scale the direction vector to some fixed length (of line)
            normalVec *= NORMAL_LINE_LEN;

            // emit another vertex (the line "tip")
            gl_Position = origVertexPos + normalVec;
            EmitVertex();

            // emit line primitve
            EndPrimitive();
        }

        void main()
        {
            GenerateLine(0); // first vertex normal
            GenerateLine(1); // second vertex normal
            GenerateLine(2); // third vertex normal
        }
    )";

    constexpr CStringView c_geometry_shader_frag_src = R"(
        #version 330 core

        // draw_normals: program that draws mesh normals
        //
        // this frag shader doesn't do much: just color each line emitted by the geometry shader
        // so that the viewers can "see" normals

        out vec4 FragColor;

        void main()
        {
            FragColor = vec4(1.0, 0.0, 0.0, 1.0);
        }
    )";

    constexpr CStringView c_vertex_shader_with_array = R"(
        #version 330 core

        layout (location = 0) in vec3 aPos;

        void main()
        {
            gl_Position = vec4(aPos, 1.0);
        }
    )";

    constexpr CStringView c_fragment_shader_with_array = R"(
        #version 330 core

        uniform vec4 uFragColor[3];

        out vec4 FragColor;

        void main()
        {
            FragColor = uFragColor[0];
        }
    )";

    // from: https://learnopengl.com/Advanced-OpenGL/Cubemaps
    constexpr CStringView c_cubemap_vertex_shader = R"(
        #version 330 core

        layout (location = 0) in vec3 aPos;

        out vec3 TexCoords;

        uniform mat4 projection;
        uniform mat4 view;

        void main()
        {
            TexCoords = aPos;
            gl_Position = projection * view * vec4(aPos, 1.0);
        }
    )";

    constexpr CStringView c_cubemap_fragment_shader = R"(
        #version 330 core

        out vec4 FragColor;

        in vec3 TexCoords;

        uniform samplerCube skybox;

        void main()
        {
            FragColor = texture(skybox, TexCoords);
        }
    )";
}

TEST_F(ShaderTest, can_be_constructed_from_vertex_and_fragment_shader_source_code)
{
    const Shader shader{c_vertex_shader_src, c_fragment_shader_src};
}

TEST_F(ShaderTest, can_be_constructed_from_vertex_geometry_and_fragment_shader_source_code)
{
    const Shader shader{c_geometry_shader_vert_src, c_geometry_shader_geom_src, c_geometry_shader_frag_src};
}

TEST_F(ShaderTest, can_be_copy_constructed)
{
    const Shader shader{c_vertex_shader_src, c_fragment_shader_src};
    const Shader copy{shader};  // NOLINT(performance-unnecessary-copy-initialization)
}

TEST_F(ShaderTest, can_be_move_constructed)
{
    Shader shader{c_vertex_shader_src, c_fragment_shader_src};
    const Shader move_constructed{std::move(shader)};
}

TEST_F(ShaderTest, can_be_copy_assigned)
{
    Shader lhs{c_vertex_shader_src, c_fragment_shader_src};
    const Shader rhs{c_vertex_shader_src, c_fragment_shader_src};

    lhs = rhs;
}

TEST_F(ShaderTest, can_be_move_assigned)
{
    Shader lhs{c_vertex_shader_src, c_fragment_shader_src};
    Shader rhs{c_vertex_shader_src, c_fragment_shader_src};

    lhs = std::move(rhs);
}

TEST_F(ShaderTest, copy_constructed_instance_compares_equivalent_to_copied_instance)
{
    const Shader shader{c_vertex_shader_src, c_fragment_shader_src};
    const Shader copy{shader}; // NOLINT(performance-unnecessary-copy-initialization)

    ASSERT_EQ(shader, copy);
}

TEST_F(ShaderTest, difference_shader_instances_compare_not_equal_even_if_they_have_the_same_sourcecode)
{
    // i.e. equality is reference equality, not value equality (this could be improved ;))

    const Shader s1{c_vertex_shader_src, c_fragment_shader_src};
    const Shader s2{c_vertex_shader_src, c_fragment_shader_src};

    ASSERT_NE(s1, s2);
}

TEST_F(ShaderTest, can_be_written_to_a_std_ostream)
{
    const Shader shader{c_vertex_shader_src, c_fragment_shader_src};

    std::stringstream ss;
    ss << shader;  // shouldn't throw etc.

    ASSERT_FALSE(ss.str().empty());
}

TEST_F(ShaderTest, writes_expected_content_to_a_std_ostream)
{
    // this test is flakey, but is just ensuring that the string printout has enough information
    // to help debugging etc.

    const Shader shader{c_vertex_shader_src, c_fragment_shader_src};

    std::stringstream ss;
    ss << shader;
    const std::string str{std::move(ss).str()};

    for (const auto& property_name : c_expected_property_names) {
        ASSERT_TRUE(contains(str, property_name));
    }
}

TEST_F(ShaderTest, property_index_can_find_expected_properties)
{
    const Shader shader{c_vertex_shader_src, c_fragment_shader_src};

    for (const auto& property_name : c_expected_property_names) {
        ASSERT_TRUE(shader.property_index(property_name));
    }
}

TEST_F(ShaderTest, num_properties_returns_expected_number_of_properties)
{
    // (effectively, number of properties == number of uniforms)
    const Shader shader{c_vertex_shader_src, c_fragment_shader_src};
    ASSERT_EQ(shader.num_properties(), c_expected_property_names.size());
}

TEST_F(ShaderTest, property_name_can_be_used_to_retrieve_all_property_names)
{
    static_assert(std::is_same_v<decltype(std::declval<Shader>().property_name(0)), std::string_view>);

    const Shader shader{c_vertex_shader_src, c_fragment_shader_src};

    const std::unordered_set<std::string_view> expected_names(c_expected_property_names.begin(), c_expected_property_names.end());

    std::unordered_set<std::string_view> returned_names;
    returned_names.reserve(shader.num_properties());
    for (size_t i = 0, len = shader.num_properties(); i < len; ++i) {
        returned_names.insert(shader.property_name(i));
    }

    ASSERT_EQ(returned_names, expected_names);
}

TEST_F(ShaderTest, property_name_returns_property_name_at_given_index)
{
    const Shader shader{c_vertex_shader_src, c_fragment_shader_src};

    for (const auto& property_name : c_expected_property_names) {
        const std::optional<size_t> idx = shader.property_index(property_name);
        ASSERT_TRUE(idx);
        ASSERT_EQ(shader.property_name(*idx), property_name);
    }
}

TEST_F(ShaderTest, property_name_still_works_if_the_property_is_an_array)
{
    const Shader shader{c_vertex_shader_with_array, c_fragment_shader_with_array};
    ASSERT_FALSE(shader.property_index("uFragColor[0]").has_value()) << "shouldn't expose 'raw' name";
    ASSERT_TRUE(shader.property_index("uFragColor").has_value()) << "should work, because the backend should normalize array-like uniforms to the original name (not uFragColor[0])";
}

TEST_F(ShaderTest, propety_type_returns_expected_type)
{
    const Shader shader{c_vertex_shader_src, c_fragment_shader_src};

    for (size_t i = 0; i < c_expected_property_names.size(); ++i) {
        static_assert(c_expected_property_names.size() == c_expected_property_types.size());
        const auto& property_name = c_expected_property_names[i];
        const ShaderPropertyType expected_property_type = c_expected_property_types[i];

        const std::optional<size_t> idx = shader.property_index(property_name);
        ASSERT_TRUE(idx);
        ASSERT_EQ(shader.property_type(*idx), expected_property_type);
    }
}

TEST_F(ShaderTest, property_type_for_cubemap_property_returns_SamplerCube)
{
    const Shader shader{c_cubemap_vertex_shader, c_cubemap_fragment_shader};
    const std::optional<size_t> property_index = shader.property_index("skybox");

    ASSERT_TRUE(property_index.has_value());
    ASSERT_EQ(shader.property_type(*property_index), ShaderPropertyType::SamplerCube);
}
