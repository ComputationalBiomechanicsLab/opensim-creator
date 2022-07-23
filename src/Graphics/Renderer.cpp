#include "Renderer.hpp"

#include "src/Bindings/GlmHelpers.hpp"
#include "src/Graphics/Color.hpp"
#include "src/Graphics/Gl.hpp"
#include "src/Graphics/GlGlm.hpp"
#include "src/Graphics/ShaderLocationIndex.hpp"
#include "src/Maths/AABB.hpp"
#include "src/Maths/Constants.hpp"
#include "src/Maths/Geometry.hpp"
#include "src/Maths/Transform.hpp"
#include "src/Platform/App.hpp"
#include "src/Platform/Log.hpp"
#include "src/Utils/Algorithms.hpp"
#include "src/Utils/Assertions.hpp"
#include "src/Utils/CStringView.hpp"
#include "src/Utils/DefaultConstructOnCopy.hpp"
#include "src/Utils/UID.hpp"

#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <nonstd/span.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <sstream>
#include <string>
#include <type_traits>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

// utility functions
namespace
{
    template<typename T>
    void DoCopyOnWrite(std::shared_ptr<T>& p)
    {
        if (p.use_count() == 1)
        {
            return;  // sole owner: no need to copy
        }

        p = std::make_shared<T>(*p);
    }

    void PushFloatAsBytes(float v, std::vector<std::byte>& out)
    {
        out.push_back(reinterpret_cast<std::byte*>(&v)[0]);
        out.push_back(reinterpret_cast<std::byte*>(&v)[1]);
        out.push_back(reinterpret_cast<std::byte*>(&v)[2]);
        out.push_back(reinterpret_cast<std::byte*>(&v)[3]);
    }

    void PushVecAsBytes(glm::vec3 const& v, std::vector<std::byte>& out)
    {
        PushFloatAsBytes(v.x, out);
        PushFloatAsBytes(v.y, out);
        PushFloatAsBytes(v.z, out);
    }

    void PushVecAsBytes(glm::vec2 const& v, std::vector<std::byte>& out)
    {
        PushFloatAsBytes(v.x, out);
        PushFloatAsBytes(v.y, out);
    }

    using MaterialValue = std::variant<float, glm::vec3, glm::vec4, glm::mat3, glm::mat4, int, bool, osc::experimental::Texture2D>;

    osc::experimental::ShaderType GetShaderType(MaterialValue const& v)
    {
        switch (v.index()) {
        case 0:
            static_assert(std::is_same_v<std::variant_alternative_t<0, MaterialValue>, float>);
            return osc::experimental::ShaderType::Float;
        case 1:
            static_assert(std::is_same_v<std::variant_alternative_t<1, MaterialValue>, glm::vec3>);
            return osc::experimental::ShaderType::Vec3;
        case 2:
            static_assert(std::is_same_v<std::variant_alternative_t<2, MaterialValue>, glm::vec4>);
            return osc::experimental::ShaderType::Vec4;
        case 3:
            static_assert(std::is_same_v<std::variant_alternative_t<3, MaterialValue>, glm::mat3>);
            return osc::experimental::ShaderType::Mat3;
        case 4:
            static_assert(std::is_same_v<std::variant_alternative_t<4, MaterialValue>, glm::mat4>);
            return osc::experimental::ShaderType::Mat4;
        case 5:
            static_assert(std::is_same_v<std::variant_alternative_t<5, MaterialValue>, int>);
            return osc::experimental::ShaderType::Int;
        case 6:
            static_assert(std::is_same_v<std::variant_alternative_t<6, MaterialValue>, bool>);
            return osc::experimental::ShaderType::Bool;
        case 7:
            static_assert(std::is_same_v<std::variant_alternative_t<7, MaterialValue>, osc::experimental::Texture2D>);
            return osc::experimental::ShaderType::Sampler2D;
        default:
            static_assert(std::variant_size_v<MaterialValue> == 8);
            return osc::experimental::ShaderType::Unknown;
        }
    }
}


//////////////////////////////////
//
// backend declaration
//
//////////////////////////////////

namespace osc::experimental {
    class GraphicsBackend final {
    public:
        static void DrawMesh(
            Mesh const& mesh,
            Transform const& transform,
            Material const& material,
            Camera& camera,
            std::optional<MaterialPropertyBlock> maybeMaterialPropertyBlock);

        static void FlushRenderQueue(Camera::Impl& camera);
    };
}


//////////////////////////////////
//
// texture stuff
//
//////////////////////////////////

namespace
{
    using namespace osc::experimental;

    static constexpr std::array<osc::CStringView, static_cast<std::size_t>(TextureWrapMode::TOTAL)> const g_TextureWrapModeStrings =
    {
        "Repeat",
        "Clamp",
        "Mirror",
    };

    static constexpr std::array<osc::CStringView, static_cast<std::size_t>(TextureFilterMode::TOTAL)> const g_TextureFilterModeStrings =
    {
        "Nearest",
        "Linear",
    };
}

class osc::experimental::Texture2D::Impl final {
public:
    Impl(int width, int height, nonstd::span<Rgba32 const> pixelsRowByRow) :
        m_Width{std::move(width)},
        m_Height{ std::move(height) },
        m_Pixels(pixelsRowByRow.data(), pixelsRowByRow.data() + pixelsRowByRow.size())
    {
        OSC_ASSERT(width * height == pixelsRowByRow.size());
    }

    int getWidth() const
    {
        return m_Width;
    }

    int getHeight() const
    {
        return m_Height;
    }

    float getAspectRatio() const
    {
        return static_cast<float>(m_Width) / static_cast<float>(m_Height);
    }

    TextureWrapMode getWrapMode() const
    {
        return getWrapModeU();
    }

    void setWrapMode(TextureWrapMode twm)
    {
        setWrapModeU(std::move(twm));
    }

    TextureWrapMode getWrapModeU() const
    {
        return m_WrapModeU;
    }

    void setWrapModeU(TextureWrapMode twm)
    {
        m_WrapModeU = std::move(twm);
    }

    TextureWrapMode getWrapModeV() const
    {
        return m_WrapModeV;
    }

    void setWrapModeV(TextureWrapMode twm)
    {
        m_WrapModeV = std::move(twm);
    }

    TextureWrapMode getWrapModeW() const
    {
        return m_WrapModeW;
    }

    void setWrapModeW(TextureWrapMode twm)
    {
        m_WrapModeW = std::move(twm);
    }

    TextureFilterMode getFilterMode() const
    {
        return m_FilterMode;
    }

    void setFilterMode(TextureFilterMode tfm)
    {
        m_FilterMode = std::move(tfm);
    }

private:
    int m_Width;
    int m_Height;
    std::vector<Rgba32> m_Pixels;
    TextureWrapMode m_WrapModeU = TextureWrapMode::Repeat;
    TextureWrapMode m_WrapModeV = TextureWrapMode::Repeat;
    TextureWrapMode m_WrapModeW = TextureWrapMode::Repeat;
    TextureFilterMode m_FilterMode = TextureFilterMode::Nearest;
};

std::ostream& osc::experimental::operator<<(std::ostream& o, TextureWrapMode twm)
{
    return o << g_TextureWrapModeStrings.at(static_cast<int>(twm));
}

std::ostream& osc::experimental::operator<<(std::ostream& o, TextureFilterMode twm)
{
    return o << g_TextureFilterModeStrings.at(static_cast<int>(twm));
}


osc::experimental::Texture2D::Texture2D(int width, int height, nonstd::span<Rgba32 const> pixelsRowByRow) :
    m_Impl{std::make_shared<Impl>(std::move(width), std::move(height), std::move(pixelsRowByRow))}
{
}

osc::experimental::Texture2D::Texture2D(Texture2D const&) = default;
osc::experimental::Texture2D::Texture2D(Texture2D&&) noexcept = default;
osc::experimental::Texture2D& osc::experimental::Texture2D::operator=(Texture2D const&) = default;
osc::experimental::Texture2D& osc::experimental::Texture2D::operator=(Texture2D&&) noexcept = default;
osc::experimental::Texture2D::~Texture2D() noexcept = default;

int osc::experimental::Texture2D::getWidth() const
{
    return m_Impl->getWidth();
}

int osc::experimental::Texture2D::getHeight() const
{
    return m_Impl->getHeight();
}

float osc::experimental::Texture2D::getAspectRatio() const
{
    return m_Impl->getAspectRatio();
}

osc::experimental::TextureWrapMode osc::experimental::Texture2D::getWrapMode() const
{
    return m_Impl->getWrapMode();
}

void osc::experimental::Texture2D::setWrapMode(TextureWrapMode twm)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setWrapMode(std::move(twm));
}

osc::experimental::TextureWrapMode osc::experimental::Texture2D::getWrapModeU() const
{
    return m_Impl->getWrapModeU();
}

void osc::experimental::Texture2D::setWrapModeU(TextureWrapMode twm)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setWrapModeU(std::move(twm));
}

osc::experimental::TextureWrapMode osc::experimental::Texture2D::getWrapModeV() const
{
    return m_Impl->getWrapModeV();
}

void osc::experimental::Texture2D::setWrapModeV(TextureWrapMode twm)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setWrapModeV(std::move(twm));
}

osc::experimental::TextureWrapMode osc::experimental::Texture2D::getWrapModeW() const
{
    return m_Impl->getWrapModeW();
}

void osc::experimental::Texture2D::setWrapModeW(TextureWrapMode twm)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setWrapModeW(std::move(twm));
}

osc::experimental::TextureFilterMode osc::experimental::Texture2D::getFilterMode() const
{
    return m_Impl->getFilterMode();
}

void osc::experimental::Texture2D::setFilterMode(TextureFilterMode twm)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setFilterMode(std::move(twm));
}

bool osc::experimental::operator==(Texture2D const& a, Texture2D const& b)
{
    return a.m_Impl == b.m_Impl;
}

bool osc::experimental::operator!=(Texture2D const& a, Texture2D const& b)
{
    return a.m_Impl != b.m_Impl;
}

bool osc::experimental::operator<(Texture2D const& a, Texture2D const& b)
{
    return a.m_Impl < b.m_Impl;
}

std::ostream& osc::experimental::operator<<(std::ostream& o, Texture2D const&)
{
    return o << "Texture2D()";
}


//////////////////////////////////
//
// shader stuff
//
//////////////////////////////////

namespace
{
    using namespace osc::experimental;

    // LUT for human-readable form of the above
    static constexpr std::array<osc::CStringView, static_cast<std::size_t>(ShaderType::TOTAL)> const g_ShaderTypeInternalStrings =
    {
        "Float",
        "Vec3",
        "Vec4",
        "Mat3",
        "Mat4",
        "Int",
        "Bool",
        "Sampler2D",
        "Unknown",
    };

    // convert a GL shader type to an internal shader type
    ShaderType GLShaderTypeToShaderTypeInternal(GLenum e)
    {
        switch (e) {
        case GL_FLOAT:
            return ShaderType::Float;
        case GL_FLOAT_VEC3:
            return ShaderType::Vec3;
        case GL_FLOAT_VEC4:
            return ShaderType::Vec4;
        case GL_FLOAT_MAT3:
            return ShaderType::Mat3;
        case GL_FLOAT_MAT4:
            return ShaderType::Mat4;
        case GL_INT:
            return ShaderType::Int;
        case GL_BOOL:
            return ShaderType::Bool;
        case GL_SAMPLER_2D:
            return ShaderType::Sampler2D;
        case GL_INT_VEC2:
        case GL_INT_VEC3:
        case GL_INT_VEC4:
        case GL_UNSIGNED_INT:
        case GL_UNSIGNED_INT_VEC2:
        case GL_UNSIGNED_INT_VEC3:
        case GL_UNSIGNED_INT_VEC4:
        case GL_DOUBLE:
        case GL_DOUBLE_VEC2:
        case GL_DOUBLE_VEC3:
        case GL_DOUBLE_VEC4:
        case GL_DOUBLE_MAT2:
        case GL_DOUBLE_MAT3:
        case GL_DOUBLE_MAT4:
        case GL_DOUBLE_MAT2x3:
        case GL_DOUBLE_MAT2x4:
        case GL_FLOAT_MAT2x3:
        case GL_FLOAT_MAT2x4:
        case GL_FLOAT_MAT3x2:
        case GL_FLOAT_MAT3x4:
        case GL_FLOAT_MAT4x2:
        case GL_FLOAT_MAT4x3:
        case GL_FLOAT_MAT2:
        case GL_FLOAT_VEC2:
        default:
            return ShaderType::Unknown;
        }
    }

    // parsed-out description of a shader "element" (uniform/attribute)
    struct ShaderElement final {
        ShaderElement(int location_, ShaderType type_) :
            location{std::move(location_)},
            type{std::move(type_)}
        {
        }

        int location;
        ShaderType type;
    };
}

class osc::experimental::Shader::Impl final {
public:
    explicit Impl(char const* vertexShader, char const* fragmentShader) :
        m_Program{gl::CreateProgramFrom(gl::CompileFromSource<gl::VertexShader>(vertexShader), gl::CompileFromSource<gl::FragmentShader>(fragmentShader))}
    {
        constexpr GLsizei maxNameLen = 16;

        GLint numAttrs;
        glGetProgramiv(m_Program.get(), GL_ACTIVE_ATTRIBUTES, &numAttrs);

        GLint numUniforms;
        glGetProgramiv(m_Program.get(), GL_ACTIVE_UNIFORMS, &numUniforms);

        m_Attributes.reserve(numAttrs);
        for (GLint i = 0; i < numAttrs; i++)
        {
            GLint size; // size of the variable
            GLenum type; // type of the variable (float, vec3 or mat4, etc)
            GLchar name[maxNameLen]; // variable name in GLSL
            GLsizei length; // name length
            glGetActiveAttrib(m_Program.get() , (GLuint)i, maxNameLen, &length, &size, &type, name);
            int location = static_cast<int>(glGetAttribLocation(m_Program.get(), name));
            m_Attributes.try_emplace(name, location, GLShaderTypeToShaderTypeInternal(type));
        }

        m_Uniforms.reserve(numUniforms);
        for (GLint i = 0; i < numUniforms; i++)
        {
            GLint size; // size of the variable
            GLenum type; // type of the variable (float, vec3 or mat4, etc)
            GLchar name[maxNameLen]; // variable name in GLSL
            GLsizei length; // name length
            glGetActiveUniform(m_Program.get(), (GLuint)i, maxNameLen, &length, &size, &type, name);
            int location = static_cast<int>(glGetUniformLocation(m_Program.get(), name));
            m_Uniforms.try_emplace(name, location, GLShaderTypeToShaderTypeInternal(type));
        }
    }

    std::optional<int> findPropertyIndex(std::string const& propertyName) const
    {
        auto it = m_Uniforms.find(propertyName);
        if (it != m_Uniforms.end())
        {
            return static_cast<int>(std::distance(m_Uniforms.begin(), it));
        }
        else
        {
            return std::nullopt;
        }
    }

    int getPropertyCount() const
    {
        return static_cast<int>(m_Uniforms.size());
    }

    std::string const& getPropertyName(int i) const
    {
        auto it = m_Uniforms.begin();
        std::advance(it, i);
        return it->first;
    }

    ShaderType getPropertyType(int i) const
    {
        auto it = m_Uniforms.begin();
        std::advance(it, i);
        return it->second.type;
    }

    // non-PIMPL APIs

    gl::Program& updProgram()
    {
        return m_Program;
    }

    std::unordered_map<std::string, ShaderElement> const& getUniforms() const
    {
        return m_Uniforms;
    }

    std::unordered_map<std::string, ShaderElement> const& getAttributes() const
    {
        return m_Attributes;
    }

private:
    UID m_UID;
    gl::Program m_Program;
    std::unordered_map<std::string, ShaderElement> m_Uniforms;
    std::unordered_map<std::string, ShaderElement> m_Attributes;
};


std::ostream& osc::experimental::operator<<(std::ostream& o, ShaderType shaderType)
{
    return o << g_ShaderTypeInternalStrings.at(static_cast<int>(shaderType));
}

osc::experimental::Shader::Shader(char const* vertexShader, char const* fragmentShader) :
    m_Impl{std::make_shared<Impl>(std::move(vertexShader), std::move(fragmentShader))}
{
}

osc::experimental::Shader::Shader(osc::experimental::Shader const&) = default;
osc::experimental::Shader::Shader(osc::experimental::Shader&&) noexcept = default;
osc::experimental:: Shader& osc::experimental::Shader::operator=(osc::experimental::Shader const&) = default;
osc::experimental::Shader& osc::experimental::Shader::operator=(osc::experimental::Shader&&) noexcept = default;
osc::experimental::Shader::~Shader() noexcept = default;

std::optional<int> osc::experimental::Shader::findPropertyIndex(std::string const& propertyName) const
{
    return m_Impl->findPropertyIndex(propertyName);
}

int osc::experimental::Shader::getPropertyCount() const
{
    return m_Impl->getPropertyCount();
}

std::string const& osc::experimental::Shader::getPropertyName(int propertyIndex) const
{
    return m_Impl->getPropertyName(std::move(propertyIndex));
}

osc::experimental::ShaderType osc::experimental::Shader::getPropertyType(int propertyIndex) const
{
    return m_Impl->getPropertyType(std::move(propertyIndex));
}

bool osc::experimental::operator==(Shader const& a, Shader const& b)
{
    return a.m_Impl == b.m_Impl;
}

bool osc::experimental::operator!=(Shader const& a, Shader const& b)
{
    return a.m_Impl != b.m_Impl;
}

bool osc::experimental::operator<(Shader const& a, Shader const& b)
{
    return a.m_Impl < b.m_Impl;
}

std::ostream& osc::experimental::operator<<(std::ostream& o, Shader const& shader)
{
    o << "Shader(\n";
    {
        o << "  uniforms = [";

        char const* delim = "\n    ";
        for (auto const& [name, data] : shader.m_Impl->getUniforms())
        {
            o << delim << "{name = " << name << ", location = " << data.location << ", type = " << data.type << '}';
        }

        o << "\n  ],\n";
    }

    {
        o << "  attributes = [";

        char const* delim = "\n    ";
        for (auto const& [name, data] : shader.m_Impl->getAttributes())
        {
            o << delim << "{name = " << name << ", location = " << data.location << ", type = " << data.type << '}';
        }

        o << "\n  ]\n";
    }

    o << ')';

    return o;
}


//////////////////////////////////
//
// material stuff
//
//////////////////////////////////

class osc::experimental::Material::Impl final {
public:
    Impl(osc::experimental::Shader shader) : m_Shader{std::move(shader)}
    {
    }

    Shader const& getShader() const
    {
        return m_Shader;
    }

    std::optional<float> getFloat(std::string_view propertyName) const
    {
        return getValue<float>(std::move(propertyName));
    }

    void setFloat(std::string_view propertyName, float value)
    {
        setValue(std::move(propertyName), value);
    }

    std::optional<glm::vec3> getVec3(std::string_view propertyName) const
    {
        return getValue<glm::vec3>(std::move(propertyName));
    }

    void setVec3(std::string_view propertyName, glm::vec3 value)
    {
        setValue(std::move(propertyName), value);
    }

    std::optional<glm::vec4> getVec4(std::string_view propertyName) const
    {
        return getValue<glm::vec4>(std::move(propertyName));
    }

    void setVec4(std::string_view propertyName, glm::vec4 value)
    {
        setValue(std::move(propertyName), value);
    }

    std::optional<glm::mat3> getMat3(std::string_view propertyName) const
    {
        return getValue<glm::mat3>(std::move(propertyName));
    }

    void setMat3(std::string_view propertyName, glm::mat3 const& value)
    {
        setValue(std::move(propertyName), value);
    }

    std::optional<glm::mat4> getMat4(std::string_view propertyName) const
    {
        return getValue<glm::mat4>(std::move(propertyName));
    }

    void setMat4(std::string_view propertyName, glm::mat4 const& value)
    {
        setValue(std::move(propertyName), value);
    }

    std::optional<int> getInt(std::string_view propertyName) const
    {
        return getValue<int>(std::move(propertyName));
    }

    void setInt(std::string_view propertyName, int value)
    {
        setValue(std::move(propertyName), value);
    }

    std::optional<bool> getBool(std::string_view propertyName) const
    {
        return getValue<bool>(std::move(propertyName));
    }

    void setBool(std::string_view propertyName, bool value)
    {
        setValue(std::move(propertyName), value);
    }

    std::optional<Texture2D> getTexture(std::string_view propertyName) const
    {
        return getValue<Texture2D>(std::move(propertyName));
    }

    void setTexture(std::string_view propertyName, Texture2D t)
    {
        setValue(std::move(propertyName), std::move(t));
    }

private:
    template<typename T>
    std::optional<T> getValue(std::string_view propertyName) const
    {
        auto it = m_Values.find(std::string{std::move(propertyName)});

        if (it == m_Values.end())
        {
            return std::nullopt;
        }

        if (!std::holds_alternative<T>(it->second))
        {
            return std::nullopt;
        }

        return std::get<T>(it->second);
    }

    template<typename T>
    void setValue(std::string_view propertyName, T const& v)
    {
        m_Values[std::string{propertyName}] = v;
    }

    friend class GraphicsBackend;

    UID m_UID;
    osc::experimental::Shader m_Shader;
    std::unordered_map<std::string, MaterialValue> m_Values;
};

osc::experimental::Material::Material(osc::experimental::Shader shader) :
    m_Impl{std::make_shared<Impl>(std::move(shader))}
{
}

osc::experimental::Material::Material(Material const&) = default;
osc::experimental::Material::Material(Material&&) noexcept = default;
osc::experimental::Material& osc::experimental::Material::operator=(Material const&) = default;
osc::experimental::Material& osc::experimental::Material::operator=(Material&&) noexcept = default;
osc::experimental::Material::~Material() noexcept = default;

osc::experimental::Shader const& osc::experimental::Material::getShader() const
{
    return m_Impl->getShader();
}

std::optional<float> osc::experimental::Material::getFloat(std::string_view propertyName) const
{
    return m_Impl->getFloat(std::move(propertyName));
}

void osc::experimental::Material::setFloat(std::string_view propertyName, float value)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setFloat(std::move(propertyName), std::move(value));
}

std::optional<glm::vec3> osc::experimental::Material::getVec3(std::string_view propertyName) const
{
    return m_Impl->getVec3(std::move(propertyName));
}

void osc::experimental::Material::setVec3(std::string_view propertyName, glm::vec3 value)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setVec3(std::move(propertyName), std::move(value));
}

std::optional<glm::vec4> osc::experimental::Material::getVec4(std::string_view propertyName) const
{
    return m_Impl->getVec4(std::move(propertyName));
}

void osc::experimental::Material::setVec4(std::string_view propertyName, glm::vec4 value)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setVec4(std::move(propertyName), std::move(value));
}

std::optional<glm::mat3> osc::experimental::Material::getMat3(std::string_view propertyName) const
{
    return m_Impl->getMat3(std::move(propertyName));
}

void osc::experimental::Material::setMat3(std::string_view propertyName, glm::mat3 const& mat)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setMat3(std::move(propertyName), mat);
}

std::optional<glm::mat4> osc::experimental::Material::getMat4(std::string_view propertyName) const
{
    return m_Impl->getMat4(std::move(propertyName));
}

void osc::experimental::Material::setMat4(std::string_view propertyName, glm::mat4 const& mat)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setMat4(std::move(propertyName), mat);
}

std::optional<int> osc::experimental::Material::getInt(std::string_view propertyName) const
{
    return m_Impl->getInt(std::move(propertyName));
}

void osc::experimental::Material::setInt(std::string_view propertyName, int value)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setInt(std::move(propertyName), std::move(value));
}

std::optional<bool> osc::experimental::Material::getBool(std::string_view propertyName) const
{
    return m_Impl->getBool(std::move(propertyName));
}

void osc::experimental::Material::setBool(std::string_view propertyName, bool value)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setBool(std::move(propertyName), std::move(value));
}

std::optional<Texture2D> osc::experimental::Material::getTexture(std::string_view propertyName) const
{
    return m_Impl->getTexture(std::move(propertyName));
}

void osc::experimental::Material::setTexture(std::string_view propertyName, Texture2D t)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setTexture(std::move(propertyName), std::move(t));
}

bool osc::experimental::operator==(Material const& a, Material const& b)
{
    return a.m_Impl == b.m_Impl;
}

bool osc::experimental::operator!=(Material const& a, Material const& b)
{
    return a.m_Impl != b.m_Impl;
}

bool osc::experimental::operator<(Material const& a, Material const& b)
{
    return a.m_Impl < b.m_Impl;
}

std::ostream& osc::experimental::operator<<(std::ostream& o, Material const&)
{
    return o << "Material()";
}


//////////////////////////////////
//
// material property block stuff
//
//////////////////////////////////

class osc::experimental::MaterialPropertyBlock::Impl final {
public:
    void clear()
    {
        m_Values.clear();
    }

    bool isEmpty() const
    {
        return m_Values.empty();
    }

    std::optional<float> getFloat(std::string_view propertyName) const
    {
        return getValue<float>(std::move(propertyName));
    }

    void setFloat(std::string_view propertyName, float value)
    {
        setValue(std::move(propertyName), value);
    }

    std::optional<glm::vec3> getVec3(std::string_view propertyName) const
    {
        return getValue<glm::vec3>(std::move(propertyName));
    }

    void setVec3(std::string_view propertyName, glm::vec3 value)
    {
        setValue(std::move(propertyName), value);
    }

    std::optional<glm::vec4> getVec4(std::string_view propertyName) const
    {
        return getValue<glm::vec4>(std::move(propertyName));
    }

    void setVec4(std::string_view propertyName, glm::vec4 value)
    {
        setValue(std::move(propertyName), value);
    }

    std::optional<glm::mat3> getMat3(std::string_view propertyName) const
    {
        return getValue<glm::mat3>(std::move(propertyName));
    }

    void setMat3(std::string_view propertyName, glm::mat3 const& value)
    {
        setValue(std::move(propertyName), value);
    }

    std::optional<glm::mat4> getMat4(std::string_view propertyName) const
    {
        return getValue<glm::mat4>(std::move(propertyName));
    }

    void setMat4(std::string_view propertyName, glm::mat4 const& value)
    {
        setValue(std::move(propertyName), value);
    }

    std::optional<int> getInt(std::string_view propertyName) const
    {
        return getValue<int>(std::move(propertyName));
    }

    void setInt(std::string_view propertyName, int value)
    {
        setValue(std::move(propertyName), value);
    }

    std::optional<bool> getBool(std::string_view propertyName) const
    {
        return getValue<bool>(std::move(propertyName));
    }

    void setBool(std::string_view propertyName, bool value)
    {
        setValue(std::move(propertyName), value);
    }

    std::optional<Texture2D> getTexture(std::string_view propertyName) const
    {
        return getValue<Texture2D>(std::move(propertyName));
    }

    void setTexture(std::string_view propertyName, Texture2D t)
    {
        setValue(std::move(propertyName), std::move(t));
    }

private:
    template<typename T>
    std::optional<T> getValue(std::string_view propertyName) const
    {
        auto it = m_Values.find(std::string{std::move(propertyName)});

        if (it == m_Values.end())
        {
            return std::nullopt;
        }

        if (!std::holds_alternative<T>(it->second))
        {
            return std::nullopt;
        }

        return std::get<T>(it->second);
    }

    template<typename T>
    void setValue(std::string_view propertyName, T const& v)
    {
        m_Values[std::string{propertyName}] = v;
    }

    friend class GraphicsBackend;

    UID m_UID;
    std::unordered_map<std::string, MaterialValue> m_Values;
};

osc::experimental::MaterialPropertyBlock::MaterialPropertyBlock() :
    m_Impl{std::make_shared<Impl>()}
{
}

osc::experimental::MaterialPropertyBlock::MaterialPropertyBlock(MaterialPropertyBlock const&) = default;
osc::experimental::MaterialPropertyBlock::MaterialPropertyBlock(MaterialPropertyBlock&&) noexcept = default;
osc::experimental::MaterialPropertyBlock& osc::experimental::MaterialPropertyBlock::operator=(MaterialPropertyBlock const&) = default;
osc::experimental::MaterialPropertyBlock& osc::experimental::MaterialPropertyBlock::operator=(MaterialPropertyBlock&&) noexcept = default;
osc::experimental::MaterialPropertyBlock::~MaterialPropertyBlock() noexcept = default;

void osc::experimental::MaterialPropertyBlock::clear()
{
    DoCopyOnWrite(m_Impl);
    m_Impl->clear();
}

bool osc::experimental::MaterialPropertyBlock::isEmpty() const
{
    return m_Impl->isEmpty();
}

std::optional<float> osc::experimental::MaterialPropertyBlock::getFloat(std::string_view propertyName) const
{
    return m_Impl->getFloat(std::move(propertyName));
}

void osc::experimental::MaterialPropertyBlock::setFloat(std::string_view propertyName, float value)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setFloat(std::move(propertyName), std::move(value));
}

std::optional<glm::vec3> osc::experimental::MaterialPropertyBlock::getVec3(std::string_view propertyName) const
{
    return m_Impl->getVec3(std::move(propertyName));
}

void osc::experimental::MaterialPropertyBlock::setVec3(std::string_view propertyName, glm::vec3 value)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setVec3(std::move(propertyName), std::move(value));
}

std::optional<glm::vec4> osc::experimental::MaterialPropertyBlock::getVec4(std::string_view propertyName) const
{
    return m_Impl->getVec4(std::move(propertyName));
}

void osc::experimental::MaterialPropertyBlock::setVec4(std::string_view propertyName, glm::vec4 value)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setVec4(std::move(propertyName), std::move(value));
}

std::optional<glm::mat3> osc::experimental::MaterialPropertyBlock::getMat3(std::string_view propertyName) const
{
    return m_Impl->getMat3(std::move(propertyName));
}

void osc::experimental::MaterialPropertyBlock::setMat3(std::string_view propertyName, glm::mat3 const& value)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setMat3(std::move(propertyName), value);
}

std::optional<glm::mat4> osc::experimental::MaterialPropertyBlock::getMat4(std::string_view propertyName) const
{
    return m_Impl->getMat4(std::move(propertyName));
}

void osc::experimental::MaterialPropertyBlock::setMat4(std::string_view propertyName, glm::mat4 const& value)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setMat4(std::move(propertyName), value);
}

std::optional<int> osc::experimental::MaterialPropertyBlock::getInt(std::string_view propertyName) const
{
    return m_Impl->getInt(std::move(propertyName));
}

void osc::experimental::MaterialPropertyBlock::setInt(std::string_view propertyName, int value)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setInt(std::move(propertyName), std::move(value));
}

std::optional<bool> osc::experimental::MaterialPropertyBlock::getBool(std::string_view propertyName) const
{
    return m_Impl->getBool(std::move(propertyName));
}

void osc::experimental::MaterialPropertyBlock::setBool(std::string_view propertyName, bool value)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setBool(std::move(propertyName), std::move(value));
}

std::optional<Texture2D> osc::experimental::MaterialPropertyBlock::getTexture(std::string_view propertyName) const
{
    return m_Impl->getTexture(std::move(propertyName));
}

void osc::experimental::MaterialPropertyBlock::setTexture(std::string_view propertyName, Texture2D t)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setTexture(std::move(propertyName), std::move(t));
}

bool osc::experimental::operator==(MaterialPropertyBlock const& a, MaterialPropertyBlock const& b)
{
    return a.m_Impl == b.m_Impl;
}

bool osc::experimental::operator!=(MaterialPropertyBlock const& a, MaterialPropertyBlock const& b)
{
    return a.m_Impl != b.m_Impl;
}

bool osc::experimental::operator<(MaterialPropertyBlock const& a, MaterialPropertyBlock const& b)
{
    return a.m_Impl < b.m_Impl;
}

std::ostream& osc::experimental::operator<<(std::ostream& o, MaterialPropertyBlock const&)
{
    return o << "MaterialPropertyBlock()";
}


//////////////////////////////////
//
// mesh stuff
//
//////////////////////////////////

namespace
{
    static constexpr std::array<osc::CStringView, 2> g_MeshTopographyStrings =
    {
        "Triangles",
        "Lines",
    };

    union PackedIndex {
        uint32_t u32;
        struct U16Pack { uint16_t a; uint16_t b; } u16;
    };

    static_assert(sizeof(PackedIndex) == sizeof(uint32_t));
    static_assert(alignof(PackedIndex) == alignof(uint32_t));

    enum class IndexFormat {
        UInt16,
        UInt32,
    };

    // the mesh data that's actually stored on the GPU
    struct MeshGPUBuffers final {
        osc::UID DataVersion;
        gl::TypedBufferHandle<GL_ARRAY_BUFFER> ArrayBuffer;
        gl::TypedBufferHandle<GL_ELEMENT_ARRAY_BUFFER> IndicesBuffer;
        gl::VertexArray VAO;
    };

    GLenum ToOpenGLTopography(osc::experimental::MeshTopography t)
    {
        switch (t) {
        case osc::experimental::MeshTopography::Triangles:
            return GL_TRIANGLES;
        case osc::experimental::MeshTopography::Lines:
            return GL_LINES;
        default:
            throw std::runtime_error{"unsuppored topography"};
        }
    }
}

class osc::experimental::Mesh::Impl final {
public:

    MeshTopography getTopography() const
    {
        return m_Topography;
    }

    void setTopography(MeshTopography t)
    {
        m_Topography = std::move(t);
        m_Version->reset();
    }

    nonstd::span<glm::vec3 const> getVerts() const
    {
        return m_Vertices;
    }

    void setVerts(nonstd::span<glm::vec3 const> verts)
    {
        m_Vertices.clear();
        m_Vertices.reserve(verts.size());
        std::copy(verts.begin(), verts.end(), std::back_insert_iterator{m_Vertices});

        recalculateBounds();
        m_Version->reset();
    }

    nonstd::span<glm::vec3 const> getNormals() const
    {
        return m_Normals;
    }

    void setNormals(nonstd::span<glm::vec3 const> normals)
    {
        m_Normals.clear();
        m_Normals.reserve(normals.size());
        std::copy(normals.begin(), normals.end(), std::back_insert_iterator{m_Normals});

        m_Version->reset();
    }

    nonstd::span<glm::vec2 const> getTexCoords() const
    {
        return m_TexCoords;
    }

    void setTexCoords(nonstd::span<glm::vec2 const> coords)
    {
        m_TexCoords.clear();
        m_TexCoords.reserve(coords.size());
        std::copy(coords.begin(), coords.end(), std::back_insert_iterator{m_TexCoords});

        m_Version->reset();
    }

    int getNumIndices() const
    {
        return m_NumIndices;
    }

    std::vector<std::uint32_t> getIndices() const
    {
        std::vector<std::uint32_t> rv;

        if (m_NumIndices <= 0)
        {
            return rv;
        }

        rv.reserve(m_NumIndices);

        if (m_IndicesAre32Bit)
        {
            nonstd::span<uint32_t const> data(&m_IndicesData.front().u32, m_NumIndices);
            std::copy(data.begin(), data.end(), std::back_insert_iterator{rv});
        }
        else
        {
            nonstd::span<std::uint16_t const> data(&m_IndicesData.front().u16.a, m_NumIndices);
            std::copy(data.begin(), data.end(), std::back_insert_iterator{rv});
        }

        return rv;
    }

    void setIndices(nonstd::span<std::uint16_t const> vs)
    {
        m_IndicesAre32Bit = false;
        m_NumIndices = static_cast<int>(vs.size());
        m_IndicesData.resize((vs.size()+1)/2);
        std::copy(vs.begin(), vs.end(), &m_IndicesData.front().u16.a);

        recalculateBounds();
        m_Version->reset();
    }

    void setIndices(nonstd::span<std::uint32_t const> vs)
    {
        auto isGreaterThanU16Max = [](uint32_t v) { return v > std::numeric_limits<uint16_t>::max(); };

        if (std::any_of(vs.begin(), vs.end(), isGreaterThanU16Max))
        {
            m_IndicesAre32Bit = true;
            m_NumIndices = static_cast<int>(vs.size());
            m_IndicesData.resize(vs.size());
            std::copy(vs.begin(), vs.end(), &m_IndicesData.front().u32);
        }
        else
        {
            m_IndicesAre32Bit = false;
            m_NumIndices = static_cast<int>(vs.size());
            m_IndicesData.resize((vs.size() + 1) / 2);
            std::copy(vs.begin(), vs.end(), &m_IndicesData.front().u16.a);
        }

        recalculateBounds();
        m_Version->reset();
    }

    AABB const& getBounds() const
    {
        return m_AABB;
    }

    void clear()
    {
        m_Version->reset();
        m_Topography = MeshTopography::Triangles;
        m_Vertices.clear();
        m_Normals.clear();
        m_TexCoords.clear();
        m_IndicesAre32Bit = false;
        m_NumIndices = 0;
        m_IndicesData.clear();
        m_AABB = {};
    }

    // non-PIMPL methods

    gl::VertexArray& updVertexArray()
    {
        if (!*m_MaybeGPUBuffers || (*m_MaybeGPUBuffers)->DataVersion != *m_Version)
        {
            uploadToGPU();
        }
        return (*m_MaybeGPUBuffers)->VAO;
    }

    void draw()
    {
        gl::DrawElements(
            ToOpenGLTopography(m_Topography),
            m_NumIndices,
            m_IndicesAre32Bit ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT,
            nullptr);
    }

    void drawInstanced(std::size_t n)
    {
        glDrawElementsInstanced(
            ToOpenGLTopography(m_Topography),
            m_NumIndices,
            m_IndicesAre32Bit ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT,
            nullptr,
            static_cast<GLsizei>(n));
    }

private:
    void recalculateBounds()
    {
        if (m_NumIndices == 0)
        {
            m_AABB = {};
        }
        else if (m_IndicesAre32Bit)
        {
            nonstd::span<uint32_t const> indices(&m_IndicesData.front().u32, m_NumIndices);
            m_AABB = osc::AABBFromIndexedVerts(m_Vertices, indices);
        }
        else
        {
            nonstd::span<uint16_t const> indices(&m_IndicesData.front().u16.a, m_NumIndices);
            m_AABB = osc::AABBFromIndexedVerts(m_Vertices, indices);
        }
    }

    void uploadToGPU()
    {
        bool hasNormals = !m_Normals.empty();
        bool hasTexCoords = !m_TexCoords.empty();
        std::size_t stride = sizeof(decltype(m_Vertices)::value_type);

        if (hasNormals)
        {
            if (m_Normals.size() != m_Vertices.size())
            {
                throw std::runtime_error{"number of normals != number of verts"};
            }
            stride += sizeof(decltype(m_Normals)::value_type);
        }

        if (hasTexCoords)
        {
            if (m_TexCoords.size() != m_Vertices.size())
            {
                throw std::runtime_error{"number of uvs != number of verts"};
            }
            stride += sizeof(decltype(m_TexCoords)::value_type);
        }

        // pack VBO data into CPU-side buffer
        std::vector<std::byte> data;
        data.reserve(stride * m_Vertices.size());

        for (std::size_t i = 0; i < m_Vertices.size(); ++i)
        {
            PushVecAsBytes(m_Vertices[i], data);
            if (hasNormals)
            {
                PushVecAsBytes(m_Normals[i], data);
            }
            if (hasTexCoords)
            {
                PushVecAsBytes(m_TexCoords[i], data);
            }
        }
        OSC_ASSERT(data.size() == stride*m_Vertices.size());

        if (!(*m_MaybeGPUBuffers))
        {
            *m_MaybeGPUBuffers = MeshGPUBuffers{};
        }
        MeshGPUBuffers& buffers = **m_MaybeGPUBuffers;

        // upload VBO data into GPU-side buffer
        gl::BindBuffer(GL_ARRAY_BUFFER, buffers.ArrayBuffer);
        gl::BufferData(GL_ARRAY_BUFFER, data.size(), data.data(), GL_STATIC_DRAW);

        // upload indices into EBO
        std::size_t eboNumBytes = m_NumIndices * (m_IndicesAre32Bit ? sizeof(std::uint32_t) : sizeof(std::uint16_t));
        gl::BindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers.IndicesBuffer);
        gl::BufferData(GL_ELEMENT_ARRAY_BUFFER, eboNumBytes, m_IndicesData.data(), GL_STATIC_DRAW);

        // create VAO, specifying layout etc.
        gl::BindVertexArray(buffers.VAO);
        gl::BindBuffer(GL_ARRAY_BUFFER, buffers.ArrayBuffer);
        gl::BindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers.IndicesBuffer);
        int offset = 0;
        glVertexAttribPointer(SHADER_LOC_VERTEX_POSITION, 3, GL_FLOAT, false, static_cast<GLsizei>(stride), reinterpret_cast<void*>(static_cast<uintptr_t>(offset)));
        glEnableVertexAttribArray(SHADER_LOC_VERTEX_POSITION);
        offset += 3 * sizeof(float);
        if (hasNormals)
        {
            glVertexAttribPointer(SHADER_LOC_VERTEX_NORMAL, 3, GL_FLOAT, false, static_cast<GLsizei>(stride), reinterpret_cast<void*>(static_cast<uintptr_t>(offset)));
            glEnableVertexAttribArray(SHADER_LOC_VERTEX_NORMAL);
            offset += 3 * sizeof(float);
        }
        if (hasTexCoords)
        {
            glVertexAttribPointer(SHADER_LOC_VERTEX_TEXCOORD01, 2, GL_FLOAT, false, static_cast<GLsizei>(stride), reinterpret_cast<void*>(static_cast<uintptr_t>(offset)));
            glEnableVertexAttribArray(SHADER_LOC_VERTEX_TEXCOORD01);
            offset += 2 * sizeof(float);
        }
        gl::BindVertexArray();

        buffers.DataVersion = *m_Version;
    }

    DefaultConstructOnCopy<UID> m_UID;
    DefaultConstructOnCopy<UID> m_Version;
    MeshTopography m_Topography = MeshTopography::Triangles;
    std::vector<glm::vec3> m_Vertices;
    std::vector<glm::vec3> m_Normals;
    std::vector<glm::vec2> m_TexCoords;

    bool m_IndicesAre32Bit = false;
    int m_NumIndices = 0;
    std::vector<PackedIndex> m_IndicesData;

    AABB m_AABB;

    DefaultConstructOnCopy<std::optional<MeshGPUBuffers>> m_MaybeGPUBuffers;
};

std::ostream& osc::experimental::operator<<(std::ostream& o, MeshTopography mt)
{
    return o << g_MeshTopographyStrings.at(static_cast<int>(mt));
}

osc::experimental::Mesh::Mesh() :
    m_Impl{std::make_shared<Impl>()}
{
}

osc::experimental::Mesh::Mesh(Mesh const&) = default;
osc::experimental::Mesh::Mesh(Mesh&&) noexcept = default;
osc::experimental::Mesh& osc::experimental::Mesh::operator=(Mesh const&) = default;
osc::experimental::Mesh& osc::experimental::Mesh::operator=(Mesh&&) noexcept = default;
osc::experimental::Mesh::~Mesh() noexcept = default;

osc::experimental::MeshTopography osc::experimental::Mesh::getTopography() const
{
    return m_Impl->getTopography();
}

void osc::experimental::Mesh::setTopography(MeshTopography topography)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setTopography(std::move(topography));
}

nonstd::span<glm::vec3 const> osc::experimental::Mesh::getVerts() const
{
    return m_Impl->getVerts();
}

void osc::experimental::Mesh::setVerts(nonstd::span<glm::vec3 const> verts)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setVerts(std::move(verts));
}

nonstd::span<glm::vec3 const> osc::experimental::Mesh::getNormals() const
{
    return m_Impl->getNormals();
}

void osc::experimental::Mesh::setNormals(nonstd::span<glm::vec3 const> verts)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setNormals(std::move(verts));
}

nonstd::span<glm::vec2 const> osc::experimental::Mesh::getTexCoords() const
{
    return m_Impl->getTexCoords();
}

void osc::experimental::Mesh::setTexCoords(nonstd::span<glm::vec2 const> coords)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setTexCoords(coords);
}

int osc::experimental::Mesh::getNumIndices() const
{
    return m_Impl->getNumIndices();
}

std::vector<std::uint32_t> osc::experimental::Mesh::getIndices() const
{
    return m_Impl->getIndices();
}

void osc::experimental::Mesh::setIndices(nonstd::span<std::uint16_t const> indices)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setIndices(std::move(indices));
}

void osc::experimental::Mesh::setIndices(nonstd::span<std::uint32_t const> indices)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setIndices(std::move(indices));
}

osc::AABB const& osc::experimental::Mesh::getBounds() const
{
    return m_Impl->getBounds();
}

void osc::experimental::Mesh::clear()
{
    DoCopyOnWrite(m_Impl);
    m_Impl->clear();
}

bool osc::experimental::operator==(Mesh const& a, Mesh const& b)
{
    return a.m_Impl == b.m_Impl;
}

bool osc::experimental::operator!=(Mesh const& a, Mesh const& b)
{
    return a.m_Impl != b.m_Impl;
}

bool osc::experimental::operator<(Mesh const& a, Mesh const& b)
{
    return a.m_Impl < b.m_Impl;
}

std::ostream& osc::experimental::operator<<(std::ostream& o, Mesh const&)
{
    return o << "Mesh()";
}


//////////////////////////////////
//
// render texture
//
//////////////////////////////////

namespace
{
    using namespace osc::experimental;

    static constexpr std::array<osc::CStringView, static_cast<std::size_t>(RenderTextureFormat::TOTAL)> const  g_RenderTextureFormatStrings =
    {
        "ARGB32",
    };

    static constexpr std::array<osc::CStringView, static_cast<std::size_t>(DepthStencilFormat::TOTAL)> const g_DepthStencilFormatStrings =
    {
        "D24_UNorm_S8_UInt",
    };
}

std::ostream& osc::experimental::operator<<(std::ostream& o, RenderTextureFormat f)
{
    return o << g_RenderTextureFormatStrings.at(static_cast<int>(f));
}

std::ostream& osc::experimental::operator<<(std::ostream& o, DepthStencilFormat f)
{
    return o << g_DepthStencilFormatStrings.at(static_cast<int>(f));
}

osc::experimental::RenderTextureDescriptor::RenderTextureDescriptor(int width, int height) :
    m_Width{width},
    m_Height{height},
    m_AnialiasingLevel{1},
    m_ColorFormat{RenderTextureFormat::ARGB32},
    m_DepthStencilFormat{DepthStencilFormat::D24_UNorm_S8_UInt}
{
}

osc::experimental::RenderTextureDescriptor::RenderTextureDescriptor(RenderTextureDescriptor const&) = default;
osc::experimental::RenderTextureDescriptor::RenderTextureDescriptor(RenderTextureDescriptor&&) noexcept = default;
osc::experimental::RenderTextureDescriptor& osc::experimental::RenderTextureDescriptor::operator=(RenderTextureDescriptor const&) = default;
osc::experimental::RenderTextureDescriptor& osc::experimental::RenderTextureDescriptor::operator=(RenderTextureDescriptor&&) noexcept = default;
osc::experimental::RenderTextureDescriptor::~RenderTextureDescriptor() noexcept = default;

int osc::experimental::RenderTextureDescriptor::getWidth() const
{
    return m_Width;
}

void osc::experimental::RenderTextureDescriptor::setWidth(int width)
{
    m_Width = width;
}

int osc::experimental::RenderTextureDescriptor::getHeight() const
{
    return m_Height;
}

void osc::experimental::RenderTextureDescriptor::setHeight(int height)
{
    m_Height = height;
}

int osc::experimental::RenderTextureDescriptor::getAntialiasingLevel() const
{
    return m_AnialiasingLevel;
}

void osc::experimental::RenderTextureDescriptor::setAntialiasingLevel(int level)
{
    m_AnialiasingLevel = level;
}

osc::experimental::RenderTextureFormat osc::experimental::RenderTextureDescriptor::getColorFormat() const
{
    return m_ColorFormat;
}

void osc::experimental::RenderTextureDescriptor::setColorFormat(RenderTextureFormat f)
{
    m_ColorFormat = f;
}

osc::experimental::DepthStencilFormat osc::experimental::RenderTextureDescriptor::getDepthStencilFormat() const
{
    return m_DepthStencilFormat;
}

void osc::experimental::RenderTextureDescriptor::setDepthStencilFormat(DepthStencilFormat f)
{
    m_DepthStencilFormat = f;
}

bool osc::experimental::operator==(RenderTextureDescriptor const& a, RenderTextureDescriptor const& b)
{
    return 
        a.m_Width == b.m_Width &&
        a.m_Height == b.m_Height &&
        a.m_AnialiasingLevel == b.m_AnialiasingLevel &&
        a.m_ColorFormat == b.m_ColorFormat &&
        a.m_DepthStencilFormat == b.m_DepthStencilFormat;
}

bool osc::experimental::operator!=(RenderTextureDescriptor const& a, RenderTextureDescriptor const& b)
{
    return !(a == b);
}

bool osc::experimental::operator<(RenderTextureDescriptor const& a, RenderTextureDescriptor const& b)
{
    return std::tie(a.m_Width, a.m_Height, a.m_AnialiasingLevel, a.m_ColorFormat, a.m_DepthStencilFormat) < std::tie(b.m_Width, b.m_Height, b.m_AnialiasingLevel, b.m_ColorFormat, b.m_DepthStencilFormat);
}

std::ostream& osc::experimental::operator<<(std::ostream& o, RenderTextureDescriptor const& rtd)
{
    return o << "RenderTextureDescriptor(width = " << rtd.m_Width << ", height = " << rtd.m_Height << ", aa = " << rtd.m_AnialiasingLevel << ", colorFormat = " << rtd.m_ColorFormat << ", depthFormat = " << rtd.m_DepthStencilFormat << ")";
}

class osc::experimental::RenderTexture::Impl final {
public:
    Impl(RenderTextureDescriptor const& desc) :
        m_Width{desc.getWidth()},
        m_Height{desc.getHeight()},
        m_AntialiasingLevel{desc.getAntialiasingLevel()},
        m_ColorFormat{desc.getColorFormat()},
        m_DepthStencilFormat{desc.getDepthStencilFormat()}
    {
    }

    int getWidth() const
    {
        return m_Width;
    }

    void setWidth(int width)
    {
        // TODO: invalidate whatever
        m_Width = width;
    }

    int getHeight() const
    {
        return m_Height;
    }

    void setHeight(int height)
    {
        // TODO: invalidate whatever
        m_Height = height;
    }

    RenderTextureFormat getColorFormat() const
    {
        return m_ColorFormat;
    }

    void setColorFormat(RenderTextureFormat format)
    {
        // TODO: invalidate whatever
        m_ColorFormat = format;
    }

    int getAntialiasingLevel() const
    {
        return m_AntialiasingLevel;
    }

    void setAntialiasingLevel(int level)
    {
        // TODO: invalidate whatever
        m_AntialiasingLevel = level;
    }

    DepthStencilFormat getDepthStencilFormat() const
    {
        return m_DepthStencilFormat;
    }

    void setDepthStencilFormat(DepthStencilFormat format)
    {
        m_DepthStencilFormat = format;
    }

private:
    int m_Width;
    int m_Height;
    int m_AntialiasingLevel;
    RenderTextureFormat m_ColorFormat;
    DepthStencilFormat m_DepthStencilFormat;
};

osc::experimental::RenderTexture::RenderTexture(RenderTextureDescriptor const& desc) :
    m_Impl{std::make_shared<Impl>(desc)}
{
}

osc::experimental::RenderTexture::RenderTexture(RenderTexture const&) = default;
osc::experimental::RenderTexture::RenderTexture(RenderTexture&&) noexcept = default;
osc::experimental::RenderTexture& osc::experimental::RenderTexture::operator=(RenderTexture const&) = default;
osc::experimental::RenderTexture& osc::experimental::RenderTexture::operator=(RenderTexture&&) noexcept = default;
osc::experimental::RenderTexture::~RenderTexture() noexcept = default;

int osc::experimental::RenderTexture::getWidth() const
{
    return m_Impl->getWidth();
}

void osc::experimental::RenderTexture::setWidth(int width)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setWidth(width);
}

int osc::experimental::RenderTexture::getHeight() const
{
    return m_Impl->getHeight();
}

void osc::experimental::RenderTexture::setHeight(int height)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setHeight(height);
}

osc::experimental::RenderTextureFormat osc::experimental::RenderTexture::getColorFormat() const
{
    return m_Impl->getColorFormat();
}

void osc::experimental::RenderTexture::setColorFormat(RenderTextureFormat format)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setColorFormat(format);
}

int osc::experimental::RenderTexture::getAntialiasingLevel() const
{
    return m_Impl->getAntialiasingLevel();
}

void osc::experimental::RenderTexture::setAntialiasingLevel(int level)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setAntialiasingLevel(level);
}

osc::experimental::DepthStencilFormat osc::experimental::RenderTexture::getDepthStencilFormat() const
{
    return m_Impl->getDepthStencilFormat();
}

void osc::experimental::RenderTexture::setDepthStencilFormat(DepthStencilFormat format)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setDepthStencilFormat(format);
}

bool osc::experimental::operator==(RenderTexture const& a, RenderTexture const& b)
{
    return a.m_Impl == b.m_Impl;
}

bool osc::experimental::operator!=(RenderTexture const& a, RenderTexture const& b)
{
    return a.m_Impl != b.m_Impl;
}

bool osc::experimental::operator<(RenderTexture const& a, RenderTexture const& b)
{
    return a.m_Impl < b.m_Impl;
}

std::ostream& osc::experimental::operator<<(std::ostream& o, RenderTexture const& rt)
{
    return o << "RenderTexture()";
}

//////////////////////////////////
//
// camera stuff
//
//////////////////////////////////

namespace
{
    using namespace osc::experimental;

    // LUT for human-readable form of the above
    static constexpr std::array<osc::CStringView, static_cast<std::size_t>(CameraProjection::TOTAL)> const g_CameraProjectionStrings =
    {
        "Perspective",
        "Orthographic",
    };
}

class osc::experimental::Camera::Impl final {
public:
    Impl() = default;

    explicit Impl(RenderTexture t) : m_MaybeTexture{std::move(t)}
    {
    }

    glm::vec4 getBackgroundColor() const
    {
        return m_BackgroundColor;
    }

    void setBackgroundColor(glm::vec4 const& color)
    {
        m_BackgroundColor = color;
    }

    CameraProjection getCameraProjection() const
    {
        return m_CameraProjection;
    }

    void setCameraProjection(CameraProjection projection)
    {
        m_CameraProjection = std::move(projection);
    }

    float getOrthographicSize() const
    {
        return m_OrthographicSize;
    }

    void setOrthographicSize(float size)
    {
        m_OrthographicSize = std::move(size);
    }

    float getCameraFOV() const
    {
        return m_PerspectiveFov;
    }

    void setCameraFOV(float size)
    {
        m_PerspectiveFov = std::move(size);
    }

    float getNearClippingPlane() const
    {
        return m_NearClippingPlane;
    }

    void setNearClippingPlane(float distance)
    {
        m_NearClippingPlane = std::move(distance);
    }

    float getFarClippingPlane() const
    {
        return m_FarClippingPlane;
    }

    void setFarClippingPlane(float distance)
    {
        m_FarClippingPlane = std::move(distance);
    }

    std::optional<RenderTexture> getTexture() const
    {
        return m_MaybeTexture;
    }

    void setTexture(RenderTexture t)
    {
        m_MaybeTexture = std::move(t);
    }

    void setTexture()
    {
        m_MaybeTexture = std::nullopt;
    }

    Rect getPixelRect() const
    {
        if (m_MaybeScreenPixelRect)
        {
            return *m_MaybeScreenPixelRect;
        }
        else
        {
            glm::vec2 appDims = osc::App::get().dims();
            return Rect{{}, appDims};
        }
    }

    void setPixelRect(Rect const& rect)
    {
        m_MaybeScreenPixelRect = rect;
    }

    int getPixelWidth() const
    {
        return getiDims().x;
    }

    int getPixelHeight() const
    {
        return getiDims().y;
    }

    float getAspectRatio() const
    {
        return AspectRatio(getiDims());
    }

    std::optional<Rect> getScissorRect() const
    {
        return m_MaybeScissorRect;
    }

    void setScissorRect(Rect const& rect)
    {
        m_MaybeScissorRect = rect;
    }

    void setScissorRect()
    {
        m_MaybeScissorRect = std::nullopt;
    }

    glm::vec3 getPosition() const
    {
        return m_Position;
    }

    void setPosition(glm::vec3 const& position)
    {
        m_Position = position;
    }

    glm::vec3 getDirection() const
    {
        return m_Direction;
    }

    void setDirection(glm::vec3 const& position)
    {
        m_Direction = position;
    }

    glm::vec3 getUpwardsDirection() const
    {
        return m_UpwardsDirection;
    }

    void setUpwardsDirection(glm::vec3 const& v)
    {
        m_UpwardsDirection = v;
    }

    glm::mat4 getViewMatrix() const
    {
        if (m_MaybeViewMatrixOverride)
        {
            return *m_MaybeViewMatrixOverride;
        }
        else
        {
            return glm::lookAt(m_Position, m_Position + m_Direction, m_UpwardsDirection);
        }
    }

    void setViewMatrix(glm::mat4 const& m)
    {
        m_MaybeViewMatrixOverride = m;
    }

    void resetViewMatrix()
    {
        m_MaybeViewMatrixOverride.reset();
    }

    glm::mat4 getProjectionMatrix() const
    {
        if (m_MaybeProjectionMatrixOverride)
        {
            return *m_MaybeProjectionMatrixOverride;
        }
        else if (m_CameraProjection == osc::experimental::CameraProjection::Perspective)
        {
            return glm::perspective(m_OrthographicSize, getAspectRatio(), m_NearClippingPlane, m_FarClippingPlane);
        }
        else
        {
            float height = m_OrthographicSize;
            float width = height * getAspectRatio();

            float right = 0.5f * width;
            float left = -right;
            float top = 0.5f * height;
            float bottom = -top;

            return glm::ortho(left, right, bottom, top, m_NearClippingPlane, m_FarClippingPlane);
        }
    }

    void setProjectionMatrix(glm::mat4 const& m)
    {
        m_MaybeProjectionMatrixOverride = m;
    }

    void resetProjectionMatrix()
    {
        m_MaybeProjectionMatrixOverride.reset();
    }

    void render()
    {
        GraphicsBackend::FlushRenderQueue(*this);
    }

private:
    glm::ivec2 getiDims() const
    {
        if (m_MaybeTexture)
        {
            return glm::ivec2{m_MaybeTexture->getWidth(), m_MaybeTexture->getHeight()};
        }
        else if (m_MaybeScreenPixelRect)
        {
            return glm::ivec2(Dimensions(*m_MaybeScreenPixelRect));
        }
        else
        {
            return osc::App::get().idims();
        }
    }

    UID m_UID;
    std::optional<RenderTexture> m_MaybeTexture = std::nullopt;
    glm::vec4 m_BackgroundColor = { 0.0f, 0.0f, 0.0f, 0.0f };
    CameraProjection m_CameraProjection = CameraProjection::Perspective;
    float m_OrthographicSize = 10.0f;
    float m_PerspectiveFov = fpi2;
    float m_NearClippingPlane = 0.01f;
    float m_FarClippingPlane = 10.0f;
    std::optional<Rect> m_MaybeScreenPixelRect = std::nullopt;
    std::optional<Rect> m_MaybeScissorRect = std::nullopt;
    glm::vec3 m_Position = {};
    glm::vec3 m_Direction = {0.0f, 0.0f, -1.0f};
    glm::vec3 m_UpwardsDirection = {0.0f, 1.0f, 0.0f};
    std::optional<glm::mat4> m_MaybeViewMatrixOverride;
    std::optional<glm::mat4> m_MaybeProjectionMatrixOverride;

    friend class GraphicsBackend;

    // renderer stuff
    struct RenderObject final {
        RenderObject(
            Mesh const& mesh_,
            Transform const& transform_,
            Material const& material_,
            std::optional<MaterialPropertyBlock> maybePropBlock_) :

            mesh{mesh_},
            transform{transform_},
            material{material_},
            maybePropBlock{std::move(maybePropBlock_)}
        {
        }

        Mesh mesh;
        Transform transform;
        Material material;
        std::optional<MaterialPropertyBlock> maybePropBlock;
    };

    std::vector<RenderObject> m_RenderQueue;
};

std::ostream& osc::experimental::operator<<(std::ostream& o, CameraProjection cp)
{
    return o << g_CameraProjectionStrings.at(static_cast<int>(cp));
}

osc::experimental::Camera::Camera() :
    m_Impl{new Impl{}}
{
}

osc::experimental::Camera::Camera(RenderTexture t) :
    m_Impl{new Impl{std::move(t)}}
{
}

osc::experimental::Camera::Camera(Camera const&) = default;
osc::experimental::Camera::Camera(Camera&&) noexcept = default;
osc::experimental::Camera& osc::experimental::Camera::operator=(Camera const&) = default;
osc::experimental::Camera& osc::experimental::Camera::operator=(Camera&&) noexcept = default;
osc::experimental::Camera::~Camera() noexcept = default;

glm::vec4 osc::experimental::Camera::getBackgroundColor() const
{
    return m_Impl->getBackgroundColor();
}

void osc::experimental::Camera::setBackgroundColor(glm::vec4 const& v)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setBackgroundColor(v);
}

osc::experimental::CameraProjection osc::experimental::Camera::getCameraProjection() const
{
    return m_Impl->getCameraProjection();
}

void osc::experimental::Camera::setCameraProjection(CameraProjection projection)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setCameraProjection(std::move(projection));
}

float osc::experimental::Camera::getOrthographicSize() const
{
    return m_Impl->getOrthographicSize();
}

void osc::experimental::Camera::setOrthographicSize(float sz)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setOrthographicSize(std::move(sz));
}

float osc::experimental::Camera::getCameraFOV() const
{
    return m_Impl->getCameraFOV();
}

void osc::experimental::Camera::setCameraFOV(float fov)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setCameraFOV(std::move(fov));
}

float osc::experimental::Camera::getNearClippingPlane() const
{
    return m_Impl->getNearClippingPlane();
}

void osc::experimental::Camera::setNearClippingPlane(float d)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setNearClippingPlane(std::move(d));
}

float osc::experimental::Camera::getFarClippingPlane() const
{
    return m_Impl->getFarClippingPlane();
}

void osc::experimental::Camera::setFarClippingPlane(float d)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setFarClippingPlane(std::move(d));
}

std::optional<osc::experimental::RenderTexture> osc::experimental::Camera::getTexture() const
{
    return m_Impl->getTexture();
}

void osc::experimental::Camera::setTexture(RenderTexture t)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setTexture(std::move(t));
}

void osc::experimental::Camera::setTexture()
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setTexture();
}

osc::Rect osc::experimental::Camera::getPixelRect() const
{
    return m_Impl->getPixelRect();
}

void osc::experimental::Camera::setPixelRect(Rect const& rect)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setPixelRect(rect);
}

int osc::experimental::Camera::getPixelWidth() const
{
    return m_Impl->getPixelWidth();
}

int osc::experimental::Camera::getPixelHeight() const
{
    return m_Impl->getPixelHeight();
}

float osc::experimental::Camera::getAspectRatio() const
{
    return m_Impl->getAspectRatio();
}

std::optional<osc::Rect> osc::experimental::Camera::getScissorRect() const
{
    return m_Impl->getScissorRect();
}

void osc::experimental::Camera::setScissorRect(Rect const& rect)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setScissorRect(rect);
}

void osc::experimental::Camera::setScissorRect()
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setScissorRect();
}

glm::vec3 osc::experimental::Camera::getPosition() const
{
    return m_Impl->getPosition();
}

void osc::experimental::Camera::setPosition(glm::vec3 const& p)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setPosition(p);
}

glm::vec3 osc::experimental::Camera::getDirection() const
{
    return m_Impl->getDirection();
}

void osc::experimental::Camera::setDirection(glm::vec3 const& dir)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setDirection(dir);
}

glm::vec3 osc::experimental::Camera::getUpwardsDirection() const
{
    return m_Impl->getUpwardsDirection();
}

void osc::experimental::Camera::setUpwardsDirection(glm::vec3 const& v)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setUpwardsDirection(v);
}

glm::mat4 osc::experimental::Camera::getViewMatrix() const
{
    return m_Impl->getViewMatrix();
}

void osc::experimental::Camera::setViewMatrix(glm::mat4 const& m)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setViewMatrix(m);
}

void osc::experimental::Camera::resetViewMatrix()
{
    DoCopyOnWrite(m_Impl);
    m_Impl->resetViewMatrix();
}

glm::mat4 osc::experimental::Camera::getProjectionMatrix() const
{
    return m_Impl->getProjectionMatrix();
}

void osc::experimental::Camera::setProjectionMatrix(glm::mat4 const& m)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setProjectionMatrix(m);
}

void osc::experimental::Camera::resetProjectionMatrix()
{
    DoCopyOnWrite(m_Impl);
    m_Impl->resetProjectionMatrix();
}

void osc::experimental::Camera::render()
{
    DoCopyOnWrite(m_Impl);
    m_Impl->render();
}

bool osc::experimental::operator==(Camera const& a, Camera const& b)
{
    return a.m_Impl == b.m_Impl;
}

bool osc::experimental::operator!=(Camera const& a, Camera const& b)
{
    return a.m_Impl != b.m_Impl;
}

bool osc::experimental::operator<(Camera const& a, Camera const& b)
{
    return a.m_Impl < b.m_Impl;
}

std::ostream& osc::experimental::operator<<(std::ostream& o, Camera const& camera)
{
    using osc::operator<<;
    return o << "Camera(position = " << camera.getPosition() << ", direction = " << camera.getDirection() << ", projection = " << camera.getCameraProjection() << ')';
}

void osc::experimental::Graphics::DrawMesh(
    Mesh const& mesh,
    Transform const& transform,
    Material const& material,
    Camera& camera,
    std::optional<MaterialPropertyBlock> maybeMaterialPropertyBlock)
{
    GraphicsBackend::DrawMesh(mesh, transform, material, camera, std::move(maybeMaterialPropertyBlock));
}



/////////////////////////
//
// backend implementation
//
/////////////////////////

void osc::experimental::GraphicsBackend::DrawMesh(
    Mesh const& mesh,
    Transform const& transform,
    Material const& material,
    Camera& camera,
    std::optional<MaterialPropertyBlock> maybeMaterialPropertyBlock)
{
    DoCopyOnWrite(camera.m_Impl);
    camera.m_Impl->m_RenderQueue.emplace_back(mesh, transform, material, std::move(maybeMaterialPropertyBlock));
}

static void TryBindMaterialValueToShaderElement(ShaderElement const& se, MaterialValue const& v)
{
    osc::experimental::ShaderType t = GetShaderType(v);

    if (GetShaderType(v) != se.type)
    {
        return;  // mismatched types
    }

    switch (t) {
    case osc::experimental::ShaderType::Float:
    {
        gl::UniformFloat u{se.location};
        gl::Uniform(u, std::get<float>(v));
        break;
    }
    case osc::experimental::ShaderType::Vec3:
    {
        gl::UniformVec3 u{se.location};
        gl::Uniform(u, std::get<glm::vec3>(v));
        break;
    }
    case osc::experimental::ShaderType::Vec4:
    {
        gl::UniformVec4 u{se.location};
        gl::Uniform(u, std::get<glm::vec4>(v));
        break;
    }
    case osc::experimental::ShaderType::Mat3:
    {
        gl::UniformMat3 u{se.location};
        gl::Uniform(u, std::get<glm::mat3>(v));
        break;
    }
    case osc::experimental::ShaderType::Mat4:
    {
        gl::UniformMat4 u{se.location};
        gl::Uniform(u, std::get<glm::mat4>(v));
        break;
    }
    case osc::experimental::ShaderType::Int:
    {
        gl::UniformInt u{se.location};
        gl::Uniform(u, std::get<int>(v));
        break;
    }
    case osc::experimental::ShaderType::Bool:
    {
        gl::UniformBool u{se.location};
        gl::Uniform(u, std::get<bool>(v));
        break;
    }
    case osc::experimental::ShaderType::Sampler2D:
    {
        throw std::runtime_error{"texture binding TODO"};
        break;
    }
    }
}

void osc::experimental::GraphicsBackend::FlushRenderQueue(Camera::Impl& camera)
{
    // top-level graphics API settings
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    gl::Enable(GL_BLEND);
    gl::Enable(GL_DEPTH_TEST);

    // set output target
    glm::ivec2 outputDimensions;
    if (camera.m_MaybeTexture)
    {
        throw std::runtime_error{"rendering to RenderTexture NYI"};

        gl::ClearColor(camera.m_BackgroundColor.r, camera.m_BackgroundColor.g, camera.m_BackgroundColor.b, camera.m_BackgroundColor.a);
        gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    }
    else
    {
        gl::BindFramebuffer(GL_FRAMEBUFFER, gl::windowFbo);
        outputDimensions = osc::App::get().idims();
        gl::Viewport(0, 0, outputDimensions.x, outputDimensions.y);
    }

    // handle scissor testing
    if (camera.m_MaybeScissorRect)
    {
        osc::Rect scissorRect = *camera.m_MaybeScissorRect;
        glm::ivec2 scissorDims = osc::Dimensions(scissorRect);

        gl::Enable(GL_SCISSOR_TEST);
        glScissor(static_cast<int>(scissorRect.p1.x), static_cast<int>(scissorRect.p1.y), scissorDims.x, scissorDims.y);
    }
    else
    {
        gl::Disable(GL_SCISSOR_TEST);
    }

    // precompute camera stuff
    glm::mat4 viewMtx = camera.getViewMatrix();
    glm::mat4 projMtx = camera.getProjectionMatrix();

    for (Camera::Impl::RenderObject const& ro : camera.m_RenderQueue)
    {
        osc::experimental::Mesh::Impl& meshImpl = const_cast<osc::experimental::Mesh::Impl&>(*ro.mesh.m_Impl);
        osc::experimental::Shader::Impl& shaderImpl = const_cast<osc::experimental::Shader::Impl&>(*ro.material.m_Impl->m_Shader.m_Impl);
        osc::experimental::Material::Impl& materialImpl = const_cast<osc::experimental::Material::Impl&>(*ro.material.m_Impl);
        osc::Transform const& transform = ro.transform;

        // TODO: polygon mode (e.g. wireframe)?

        gl::UseProgram(shaderImpl.updProgram());

        // bind uniforms

        std::unordered_map<std::string, ShaderElement> const& uniforms = shaderImpl.getUniforms();

        // try binding to uModel (standard)
        {
            auto it = uniforms.find("uModelMat");
            if (it != uniforms.end() && it->second.type == osc::experimental::ShaderType::Mat4)
            {
                gl::UniformMat4 u{it->second.location};
                gl::Uniform(u, ToMat4(transform));
            }
        }

        // try binding to uView (standard)
        {
            auto it = uniforms.find("uViewMat");
            if (it != uniforms.end() && it->second.type == osc::experimental::ShaderType::Mat4)
            {
                gl::UniformMat4 u{it->second.location};
                gl::Uniform(u, viewMtx);
            }
        }

        // try binding to uProjection (standard)
        {
            auto it = uniforms.find("uProjMat");
            if (it != uniforms.end() && it->second.type == osc::experimental::ShaderType::Mat4)
            {
                gl::UniformMat4 u{it->second.location};
                gl::Uniform(u, projMtx);
            }
        }

        // bind material values
        for (auto const& [name, value] : materialImpl.m_Values)
        {
            auto it = uniforms.find(name);
            if (it != uniforms.end())
            {
                TryBindMaterialValueToShaderElement(it->second, value);
            }
        }

        // bind material block values
        if (ro.maybePropBlock)
        {
            for (auto const& [name, value] : ro.maybePropBlock->m_Impl->m_Values)
            {
                auto it = uniforms.find(name);
                if (it != uniforms.end())
                {
                    TryBindMaterialValueToShaderElement(it->second, value);
                }
            }
        }

        gl::BindVertexArray(meshImpl.updVertexArray());
        meshImpl.draw();
        gl::BindVertexArray();

        gl::UseProgram();
    }
    gl::BindVertexArray();

    camera.m_RenderQueue.clear();
}
