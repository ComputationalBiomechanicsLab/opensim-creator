#include "Renderer.hpp"

#include "src/Graphics/Gl.hpp"
#include "src/Maths/Geometry.hpp"
#include "src/Utils/Assertions.hpp"
#include "src/Utils/CStringView.hpp"
#include "src/Utils/UID.hpp"

#include <algorithm>
#include <array>
#include <iostream>
#include <stdexcept>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#define OSC_THROW_NYI() throw std::runtime_error{"not yet implemented"}

template<typename T>
static void DoCopyOnWrite(std::shared_ptr<T>& p)
{
    if (p.use_count() == 1)
    {
        return;  // sole owner: no need to copy
    }

    p = std::make_shared<T>(*p);
}

template<typename T>
static std::string StreamToString(T const& v)
{
    std::stringstream ss;
    ss << v;
    return std::move(ss).str();
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
        OSC_ASSERT(width* height == pixelsRowByRow.size());
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

    std::size_t getHash() const
    {
        return std::hash<UID>{}(m_UID);
    }

private:
    UID m_UID;
    int m_Width;
    int m_Height;
    TextureWrapMode m_WrapModeU = TextureWrapMode::Repeat;
    TextureWrapMode m_WrapModeV = TextureWrapMode::Repeat;
    TextureWrapMode m_WrapModeW = TextureWrapMode::Repeat;
    TextureFilterMode m_FilterMode = TextureFilterMode::Nearest;
    std::vector<Rgba32> m_Pixels;
};

std::ostream& osc::experimental::operator<<(std::ostream& o, TextureWrapMode twm)
{
    return o << g_TextureWrapModeStrings.at(static_cast<int>(twm));
}

std::string osc::experimental::to_string(TextureWrapMode twm)
{
    return StreamToString(twm);
}

std::ostream& osc::experimental::operator<<(std::ostream& o, TextureFilterMode twm)
{
    return o << g_TextureFilterModeStrings.at(static_cast<int>(twm));
}

std::string osc::experimental::to_string(TextureFilterMode twm)
{
    return StreamToString(twm);
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

bool osc::experimental::operator<=(Texture2D const& a, Texture2D const& b)
{
    return a.m_Impl <= b.m_Impl;
}

bool osc::experimental::operator>(Texture2D const& a, Texture2D const& b)
{
    return a.m_Impl > b.m_Impl;
}

bool osc::experimental::operator>=(Texture2D const& a, Texture2D const& b)
{
    return a.m_Impl >= b.m_Impl;
}

std::ostream& osc::experimental::operator<<(std::ostream& o, Texture2D const& t)
{
    return o << "Texture2D";
}

std::string osc::experimental::to_string(Texture2D const& t)
{
    return StreamToString(t);
}

std::size_t std::hash<osc::experimental::Texture2D>::operator()(osc::experimental::Texture2D const& t) const
{
    return t.m_Impl->getHash();
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
        "Mat4x3",
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
        case GL_FLOAT_MAT4x3:
            return ShaderType::Mat4x3;
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
            m_Attributes.try_emplace(name, static_cast<int>(i), GLShaderTypeToShaderTypeInternal(type));
        }

        m_Uniforms.reserve(numUniforms);
        for (GLint i = 0; i < numUniforms; i++)
        {
            GLint size; // size of the variable
            GLenum type; // type of the variable (float, vec3 or mat4, etc)
            GLchar name[maxNameLen]; // variable name in GLSL
            GLsizei length; // name length
            glGetActiveUniform(m_Program.get(), (GLuint)i, maxNameLen, &length, &size, &type, name);
            m_Uniforms.try_emplace(name, static_cast<int>(i), GLShaderTypeToShaderTypeInternal(type));
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

    std::size_t getHash() const
    {
        return std::hash<osc::UID>{}(m_UID);
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

std::string osc::experimental::to_string(ShaderType shaderType)
{
    return std::string{g_ShaderTypeInternalStrings.at(static_cast<int>(shaderType))};
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

bool osc::experimental::operator<=(Shader const& a, Shader const& b)
{
    return a.m_Impl <= b.m_Impl;
}

bool osc::experimental::operator>(Shader const& a, Shader const& b)
{
    return a.m_Impl > b.m_Impl;
}

bool osc::experimental::operator>=(Shader const& a, Shader const& b)
{
    return a.m_Impl >= b.m_Impl;
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

std::string osc::experimental::to_string(Shader const& shader)
{
    std::stringstream ss;
    ss << shader;
    return std::move(ss).str();
}

std::size_t std::hash<osc::experimental::Shader>::operator()(osc::experimental::Shader const& shader) const
{
    return shader.m_Impl->getHash();
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

    std::optional<glm::mat4x3> getMat4x3(std::string_view propertyName) const
    {
        return getValue<glm::mat4x3>(std::move(propertyName));
    }

    void setMat4x3(std::string_view propertyName, glm::mat4x3 const& value)
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

    // non-PIMPL APIs

    std::size_t getHash() const
    {
        return std::hash<osc::UID>{}(m_UID);
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

    using Value = std::variant<float, glm::vec3, glm::vec4, glm::mat3, glm::mat4, glm::mat4x3, int, bool, Texture2D>;

    UID m_UID;
    osc::experimental::Shader m_Shader;
    std::unordered_map<std::string, Value> m_Values;
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

std::optional<glm::mat4x3> osc::experimental::Material::getMat4x3(std::string_view propertyName) const
{
    return m_Impl->getMat4x3(std::move(propertyName));
}

void osc::experimental::Material::setMat4x3(std::string_view propertyName, glm::mat4x3 const& mat)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setMat4x3(std::move(propertyName), mat);
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

bool osc::experimental::operator<=(Material const& a, Material const& b)
{
    return a.m_Impl <= b.m_Impl;
}

bool osc::experimental::operator>(Material const& a, Material const& b)
{
    return a.m_Impl > b.m_Impl;
}

bool osc::experimental::operator>=(Material const& a, Material const& b)
{
    return a.m_Impl >= b.m_Impl;
}

std::ostream& osc::experimental::operator<<(std::ostream& o, Material const& material)
{
    return o << "Material()";
}

std::string osc::experimental::to_string(Material const& material)
{
    return StreamToString(material);
}

std::size_t std::hash<osc::experimental::Material>::operator()(osc::experimental::Material const& material) const
{
    return material.m_Impl->getHash();
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

    std::optional<glm::mat4x3> getMat4x3(std::string_view propertyName) const
    {
        return getValue<glm::mat4x3>(std::move(propertyName));
    }

    void setMat4x3(std::string_view propertyName, glm::mat4x3 const& value)
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

    // non-PIMPL APIs

    std::size_t getHash() const
    {
        return std::hash<osc::UID>{}(m_UID);
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

    using Value = std::variant<float, glm::vec3, glm::vec4, glm::mat3, glm::mat4, glm::mat4x3, int, bool, Texture2D>;

    UID m_UID;
    std::unordered_map<std::string, Value> m_Values;
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

std::optional<glm::mat4x3> osc::experimental::MaterialPropertyBlock::getMat4x3(std::string_view propertyName) const
{
    return m_Impl->getMat4x3(std::move(propertyName));
}

void osc::experimental::MaterialPropertyBlock::setMat4x3(std::string_view propertyName, glm::mat4x3 const& value)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setMat4x3(std::move(propertyName), value);
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

bool osc::experimental::operator<=(MaterialPropertyBlock const& a, MaterialPropertyBlock const& b)
{
    return a.m_Impl <= b.m_Impl;
}

bool osc::experimental::operator>(MaterialPropertyBlock const& a, MaterialPropertyBlock const& b)
{
    return a.m_Impl > b.m_Impl;
}

bool osc::experimental::operator>=(MaterialPropertyBlock const& a, MaterialPropertyBlock const& b)
{
    return a.m_Impl >= b.m_Impl;
}

std::ostream& osc::experimental::operator<<(std::ostream& o, MaterialPropertyBlock const&)
{
    return o << "MaterialPropertyBlock()";
}

std::string osc::experimental::to_string(MaterialPropertyBlock const& material)
{
    return StreamToString(material);
}

std::size_t std::hash<osc::experimental::MaterialPropertyBlock>::operator()(osc::experimental::MaterialPropertyBlock const& mpb) const
{
    return mpb.m_Impl->getHash();
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
        m_Version = {};
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
        m_Version = {};
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

        m_Version = {};
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

        m_Version = {};
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
        m_Version = {};
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
            m_IndicesData.resize((vs.size()+1)/2);
            std::copy(vs.begin(), vs.end(), &m_IndicesData.front().u16.a);
        }

        recalculateBounds();
        m_Version = {};
    }

    AABB const& getBounds() const
    {
        return m_AABB;
    }

    void clear()
    {
        m_Version = {};
        m_Topography = MeshTopography::Triangles;
        m_Vertices.clear();
        m_Normals.clear();
        m_TexCoords.clear();
        m_IndicesAre32Bit = false;
        m_NumIndices = 0;
        m_IndicesData.clear();
        m_AABB = {};
    }

    std::size_t getHash() const
    {
        return m_UID;
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

    UID m_UID;
    UID m_Version;
    MeshTopography m_Topography = MeshTopography::Triangles;
    std::vector<glm::vec3> m_Vertices;
    std::vector<glm::vec3> m_Normals;
    std::vector<glm::vec2> m_TexCoords;

    bool m_IndicesAre32Bit = false;
    int m_NumIndices = 0;
    std::vector<PackedIndex> m_IndicesData;

    AABB m_AABB;
};

std::ostream& osc::experimental::operator<<(std::ostream& o, MeshTopography mt)
{
    return o << g_MeshTopographyStrings.at(static_cast<int>(mt));
}

std::string osc::experimental::to_string(MeshTopography mt)
{
    return StreamToString(std::move(mt));
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

bool osc::experimental::operator<=(Mesh const& a, Mesh const& b)
{
    return a.m_Impl <= b.m_Impl;
}

bool osc::experimental::operator>(Mesh const& a, Mesh const& b)
{
    return a.m_Impl > b.m_Impl;
}

bool osc::experimental::operator>=(Mesh const& a, Mesh const& b)
{
    return a.m_Impl >= b.m_Impl;
}

std::ostream& osc::experimental::operator<<(std::ostream& o, Mesh const& mesh)
{
    return o << "Mesh()";
}

std::string osc::experimental::to_string(Mesh const& mesh)
{
    return StreamToString(mesh);
}

std::size_t std::hash<osc::experimental::Mesh>::operator()(osc::experimental::Mesh const& mesh) const
{
    return mesh.m_Impl->getHash();
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

std::ostream& osc::experimental::operator<<(std::ostream& o, CameraProjection cp)
{
    return o << g_CameraProjectionStrings.at(static_cast<int>(cp));
}

std::string osc::experimental::to_string(CameraProjection cp)
{
    return StreamToString(std::move(cp));
}

/*


// 3D implementation notes
//
// - almost all public-API types are designed to be copy-on-write classes that
//   downstream code can easily store, compare, etc. "as-if" the callers were
//   dealing with value-types
//
// - the value types are comparable, assignable, etc. so that they can be easily
//   used downstream. Most of them are also printable
//
// - hashing etc. needs to ensure a unique hash value is generated even if the
//   same implementation pointer is hashed (e.g. because the caller could be
//   using the hash as a way of caching downstream data). Over-hashing is
//   preferred to under-hashing (i.e. it's preferable for the same "value" to
//   hash to something different because something like a version number was
//   incremented, even if the values are the same, because that's better than
//   emitting the same hash value for different actual values)
//
// - all the implementation details are stuffed in here because the rendering
//   classes are going to be used all over the codebase and it's feasible that
//   little internal changes will happen here as other backends, caching, etc.
//   are supported

// globals
namespace
{
    constexpr std::array<char const*, static_cast<std::size_t>(osc::experimental::MeshTopography::TOTAL)> const g_MeshTopographyStrings =
    {
        "Triangles",
        "Lines",
    };

    constexpr std::array<char const*, static_cast<std::size_t>(osc::experimental::TextureWrapMode::TOTAL)> const g_TextureWrapModeStrings =
    {
        "Repeat",
        "Clamp",
        "Mirror",
    };

    constexpr std::array<char const*, static_cast<std::size_t>(osc::experimental::TextureFilterMode::TOTAL)> const g_TextureFilterModeStrings =
    {
        "Nearest",
        "Linear",
        "Mipmap",
    };

    constexpr std::array<char const*, static_cast<std::size_t>(osc::experimental::ShaderType::TOTAL)> g_ShaderTypeStrings =
    {
        "Float",
        "Int",
        "Matrix",
        "Texture",
        "Vector",
    };

    constexpr std::array<char const*, static_cast<std::size_t>(osc::experimental::ShaderType::TOTAL)> g_CameraProjectionStrings =
    {
        "Perspective",
        "Orthographic",
    };
}


// helpers
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

    template<typename T>
    std::string StreamToString(T const& v)
    {
        std::stringstream ss;
        ss << v;
        return std::move(ss).str();
    }
}

namespace
{
    enum class IndexFormat {
        UInt16,
        UInt32,
    };

    // represents index data, which may be packed in-memory depending
    // on how it's represented
    union PackedIndex {
        uint32_t u32;
        struct U16Pack { uint16_t a; uint16_t b; } u16;
    };

    static_assert(sizeof(PackedIndex) == sizeof(uint32_t), "double-check that uint32_t is correctly represented on this machine");
    static_assert(alignof(PackedIndex) == alignof(uint32_t), "careful: the union is type-punning between 16- and 32-bit data");

    // pack u16 indices into a u16 `PackedIndex` vector
    void PackAsU16(nonstd::span<uint16_t const> vs, std::vector<PackedIndex>& out)
    {
        if (vs.empty())
        {
            out.clear();
            return;
        }

        out.resize((vs.size()+1)/2);

        std::copy(vs.data(), vs.data() + vs.size(), &out.front().u16.a);

        // zero out the second part of the packed index if there was an odd number
        // of input indices (the 2nd half will be left untouched by the copy above)
        if (vs.size() % 2)
        {
            out.back().u16.b = {};
        }
    }

    void PackAsU16(nonstd::span<uint32_t const> vs, std::vector<PackedIndex>& out)
    {
        if (vs.empty())
        {
            out.clear();
            return;
        }

        out.resize((vs.size()+1)/2);

        // pack two 32-bit source values into one `PackedIndex` per iteration
        for (std::size_t i = 0, end = vs.size()-1; i < end; i += 2)
        {
            PackedIndex& pi = out[i/2];
            pi.u16.a = static_cast<uint16_t>(vs[i]);
            pi.u16.b = static_cast<uint16_t>(vs[i+1]);
        }

        // with an odd number of source values, pack the trailing 32-bit value into
        // the first part of the packed index and zero out the second part
        if (vs.size() % 2)
        {
            PackedIndex& pi = out.back();
            pi.u16.a = static_cast<uint16_t>(vs.back());
            pi.u16.b = {};
        }
    }

    void PackAsU32(nonstd::span<uint16_t const> vs, std::vector<PackedIndex>& out)
    {
        out.clear();
        out.reserve(vs.size());

        for (uint16_t const& v : vs)
        {
            out.push_back(PackedIndex{static_cast<uint32_t>(v)});
        }
    }

    void PackAsU32(nonstd::span<uint32_t const> vs, std::vector<PackedIndex>& out)
    {
        out.clear();
        out.resize(vs.size());
        std::copy(vs.data(), vs.data() + vs.size(), &out.front().u32);
    }

    std::vector<uint32_t> UnpackU32ToU32(nonstd::span<PackedIndex const> data, int numIndices)
    {
        uint32_t const* begin = &data[0].u32;
        uint32_t const* end = begin + numIndices;
        return std::vector<uint32_t>(begin, end);
    }

    std::vector<uint32_t> UnpackU16ToU32(nonstd::span<PackedIndex const> data, int numIndices)
    {
        std::vector<uint32_t> rv;
        rv.reserve(numIndices);

        uint16_t const* it = &data[0].u16.a;
        uint16_t const* end = it + numIndices;
        for (; it != end; ++it)
        {
            rv.push_back(static_cast<uint32_t>(*it));
        }

        return rv;
    }

    std::vector<osc::Rgba32> PackAsRGBA32(nonstd::span<glm::vec4 const> pixels)
    {
        std::vector<osc::Rgba32> rv;
        rv.reserve(pixels.size());
        for (glm::vec4 const& pixel : pixels)
        {
            rv.push_back(osc::Rgba32FromVec4(pixel));
        }
        return rv;
    }

    std::vector<PackedIndex> PackIndexRangeAsU16(std::size_t n)
    {
        OSC_ASSERT(n <= std::numeric_limits<uint16_t>::max());

        std::size_t numEls = (n+1)/2;

        std::vector<PackedIndex> rv;
        rv.reserve(numEls);

        for (std::size_t i = 0, end = n-1; i < end; i += 2)
        {
            PackedIndex& pi = rv.emplace_back();
            pi.u16.a = static_cast<uint16_t>(i);
            pi.u16.b = static_cast<uint16_t>(i+1);
        }

        if (n % 2)
        {
            PackedIndex& pi = rv.emplace_back();
            pi.u16.a = static_cast<uint16_t>(n-1);
            pi.u16.b = {};
        }

        return rv;
    }

    std::vector<PackedIndex> PackIndexRangeAsU32(std::size_t n)
    {
        std::vector<PackedIndex> rv;
        rv.reserve(n);

        for (std::size_t i = 0; i < n; ++i)
        {
            PackedIndex& pi = rv.emplace_back();
            pi.u32 = static_cast<uint32_t>(i);
        }

        return rv;
    }

    template<typename T>
    static std::vector<T> Create0ToNIndices(std::size_t n)
    {
        static_assert(std::is_integral_v<T>);

        OSC_ASSERT(n >= 0);
        OSC_ASSERT(n <= std::numeric_limits<T>::max());

        std::vector<T> rv;
        rv.reserve(n);

        for (T i = 0; i < n; ++i)
        {
            rv.push_back(i);
        }

        return rv;
    }

    static nonstd::span<uint32_t const> AsU32Span(PackedIndex const* pi, size_t n)
    {
        return nonstd::span{&pi->u32, n};
    }

    static nonstd::span<uint16_t const> AsU16Span(PackedIndex const* pi, size_t n)
    {
        return nonstd::span{&pi->u16.a, n};
    }
}


class osc::experimental::Mesh::Impl final {
public:
    Impl() = default;

    Impl(MeshTopography t, nonstd::span<glm::vec3 const> verts) :
        m_Topography{std::move(t)},
        m_Verts(verts.begin(), verts.end()),
        m_IndexFormat{verts.size() <= std::numeric_limits<uint16_t>::max() ? IndexFormat::UInt16 : IndexFormat::UInt32},
        m_NumIndices{static_cast<int>(verts.size())},
        m_IndicesData{m_IndexFormat == IndexFormat::UInt16 ? PackIndexRangeAsU16(verts.size()) : PackIndexRangeAsU32(verts.size())}
    {
        recalculateBounds();
    }

    int64_t getVersion() const
    {
        return *m_Version;
    }

    MeshTopography getTopography() const
    {
        return m_Topography;
    }

    void setTopography(MeshTopography mt)
    {
        m_Topography = mt;
        (*m_Version)++;
    }

    nonstd::span<glm::vec3 const> getVerts() const
    {
        return m_Verts;
    }

    void setVerts(nonstd::span<const glm::vec3> vs)
    {
        m_Verts.clear();
        m_Verts.reserve(vs.size());

        for (glm::vec3 const& v : vs)
        {
            m_Verts.push_back(v);
        }

        m_GpuBuffersUpToDate = false;
        (*m_Version)++;
        recalculateBounds();
    }

    nonstd::span<glm::vec3 const> getNormals() const
    {
        return m_Normals;
    }

    void setNormals(nonstd::span<const glm::vec3> ns)
    {
        m_Normals.clear();
        m_Normals.reserve(ns.size());

        for (glm::vec3 const& v : ns)
        {
            m_Normals.push_back(v);
        }

        m_GpuBuffersUpToDate = false;
        (*m_Version)++;
    }

    nonstd::span<glm::vec2 const> getTexCoords() const
    {
        return m_TexCoords;
    }

    void setTexCoords(nonstd::span<const glm::vec2> tc)
    {
        m_TexCoords.clear();
        m_TexCoords.reserve(tc.size());

        for (glm::vec2 const& t : tc)
        {
            m_TexCoords.push_back(t);
        }

        m_GpuBuffersUpToDate = false;
        (*m_Version)++;
    }

    int getNumIndices() const
    {
        return m_NumIndices;
    }

    std::vector<uint32_t> getIndices() const
    {
        if (m_IndicesData.empty())
        {
            return std::vector<uint32_t>{};
        }
        else if (m_IndexFormat == IndexFormat::UInt32)
        {
            return UnpackU32ToU32(m_IndicesData, m_NumIndices);
        }
        else
        {
            return UnpackU16ToU32(m_IndicesData, m_NumIndices);
        }
    }

    template<typename T>
    void setIndices(nonstd::span<T const> vs)
    {
        if (m_IndexFormat == IndexFormat::UInt16)
        {
            PackAsU16(vs, m_IndicesData);
        }
        else
        {
            PackAsU32(vs, m_IndicesData);
        }

        m_NumIndices = static_cast<int>(vs.size());
        m_GpuBuffersUpToDate = false;
        (*m_Version)++;
        recalculateBounds();
    }

    AABB const& getBounds() const
    {
        return m_AABB;
    }

    void clear()
    {
        // m_Version : leave as-is (over-hashing is fine, under-hashing is not)
        m_Verts.clear();
        m_Normals.clear();
        m_TexCoords.clear();
        m_NumIndices = 0;
        m_IndicesData.clear();
        m_AABB = AABB{};
        m_GpuBuffersUpToDate = false;
    }

    void recalculateBounds()
    {
        if (m_IndexFormat == IndexFormat::UInt16)
        {
            m_AABB = AABBFromIndexedVerts(m_Verts, AsU16Span(m_IndicesData.data(), m_NumIndices));
        }
        else
        {
            m_AABB = AABBFromIndexedVerts(m_Verts, AsU32Span(m_IndicesData.data(), m_NumIndices));
        }
    }

    std::size_t getHash() const
    {
        return HashOf(*m_ID, *m_Version);
    }

private:
    DefaultConstructOnCopy<UID> m_ID;
    DefaultConstructOnCopy<int64_t> m_Version;
    MeshTopography m_Topography = MeshTopography::Triangles;
    std::vector<glm::vec3> m_Verts;
    std::vector<glm::vec3> m_Normals;
    std::vector<glm::vec2> m_TexCoords;
    IndexFormat m_IndexFormat = IndexFormat::UInt16;
    int m_NumIndices = 0;
    std::vector<PackedIndex> m_IndicesData;
    AABB m_AABB = {};
    DefaultConstructOnCopy<bool> m_GpuBuffersUpToDate = false;

    // TODO: GPU data
};


class osc::experimental::Texture2D::Impl final {
public:
    Impl(int width, int height, nonstd::span<Rgba32 const> pixels) :
        m_Dims{width, height},
        m_PixelData{pixels.begin(), pixels.end()}
    {
        OSC_ASSERT(static_cast<int>(m_PixelData.size()) == width*height);
    }

    Impl(int width, int height, nonstd::span<glm::vec4 const> pixels) :
        m_Dims{width, height},
        m_PixelData{PackAsRGBA32(pixels)}
    {
        OSC_ASSERT(static_cast<int>(m_PixelData.size()) == width*height);
    }

    std::int64_t getVersion() const
    {
        return *m_Version;
    }

    int getWidth() const
    {
        return m_Dims.x;
    }

    int getHeight() const
    {
        return m_Dims.y;
    }

    float getAspectRatio() const
    {
        return AspectRatio(m_Dims);
    }

    TextureWrapMode getWrapMode() const
    {
        return getWrapModeU();
    }

    void setWrapMode(TextureWrapMode wm)
    {
        setWrapModeU(wm);
        (*m_Version)++;
    }

    TextureWrapMode getWrapModeU() const
    {
        return m_WrapModeU;
    }

    void setWrapModeU(TextureWrapMode wm)
    {
        m_WrapModeU = wm;
        (*m_Version)++;
    }

    TextureWrapMode getWrapModeV() const
    {
        return m_WrapModeV;
    }

    void setWrapModeV(TextureWrapMode wm)
    {
        m_WrapModeV = wm;
        (*m_Version)++;
    }

    TextureWrapMode getWrapModeW() const
    {
        return m_WrapModeW;
    }

    void setWrapModeW(TextureWrapMode wm)
    {
        m_WrapModeW = wm;
        (*m_Version)++;
    }

    TextureFilterMode getFilterMode() const
    {
        return m_FilterMode;
    }

    void setFilterMode(TextureFilterMode fm)
    {
        m_FilterMode = fm;
        (*m_Version)++;
    }

    std::size_t getHash() const
    {
        return HashOf(*m_ID, *m_Version);
    }

private:
    DefaultConstructOnCopy<UID> m_ID;
    DefaultConstructOnCopy<int64_t> m_Version;
    glm::ivec2 m_Dims;
    std::vector<osc::Rgba32> m_PixelData;
    TextureWrapMode m_WrapModeU = TextureWrapMode::Repeat;
    TextureWrapMode m_WrapModeV = TextureWrapMode::Repeat;
    TextureWrapMode m_WrapModeW = TextureWrapMode::Repeat;
    TextureFilterMode m_FilterMode = TextureFilterMode::Linear;
    DefaultConstructOnCopy<bool> m_GpuBuffersUpToDate = false;

    // TODO: GPU data
};


class osc::experimental::Material::Impl final {
public:
    Impl(Shader shader) : m_Shader{std::move(shader)}
    {
    }

    std::int64_t getVersion() const
    {
        OSC_THROW_NYI();
    }

    Shader const& getShader() const
    {
        OSC_THROW_NYI();
    }

    bool hasProperty(std::string_view) const
    {
        OSC_THROW_NYI();
    }

    glm::vec4 const* getColor() const
    {
        OSC_THROW_NYI();
    }

    void setColor(glm::vec4 const&)
    {
        OSC_THROW_NYI();
    }

    float const* getFloat(std::string_view) const
    {
        OSC_THROW_NYI();
    }

    void setFloat(std::string_view, float)
    {
        OSC_THROW_NYI();
    }

    int const* getInt(std::string_view) const
    {
        OSC_THROW_NYI();
    }

    void setInt(std::string_view, int)
    {
        OSC_THROW_NYI();
    }

    Texture2D const* getTexture(std::string_view) const
    {
        OSC_THROW_NYI();
    }

    void setTexture(std::string_view, Texture2D const&)
    {
        OSC_THROW_NYI();
    }

    glm::vec4 const* getVector(std::string_view) const
    {
        OSC_THROW_NYI();
    }

    void setVector(std::string_view, glm::vec4 const&)
    {
        OSC_THROW_NYI();
    }

    glm::mat4 const* getMatrix(std::string_view) const
    {
        OSC_THROW_NYI();
    }

    void setMatrix(std::string_view, glm::mat4 const&)
    {
        OSC_THROW_NYI();
    }

    std::size_t getHash() const
    {
        OSC_THROW_NYI();
    }

private:
    Shader m_Shader;
};


class osc::experimental::MaterialPropertyBlock::Impl final {
public:
    std::int64_t getVersion() const
    {
        OSC_THROW_NYI();
    }

    void clear()
    {
        OSC_THROW_NYI();
    }

    bool isEmpty() const
    {
        OSC_THROW_NYI();
    }

    bool hasProperty(std::string_view) const
    {
        OSC_THROW_NYI();
    }

    glm::vec4 const* getColor() const
    {
        OSC_THROW_NYI();
    }

    void setColor(glm::vec4 const&)
    {
        OSC_THROW_NYI();
    }

    float const* getFloat(std::string_view) const
    {
        OSC_THROW_NYI();
    }

    void setFloat(std::string_view, float)
    {
        OSC_THROW_NYI();
    }

    int const* getInt(std::string_view) const
    {
        OSC_THROW_NYI();
    }

    void setInt(std::string_view, int)
    {
        OSC_THROW_NYI();
    }

    Texture2D const* getTexture(std::string_view) const
    {
        OSC_THROW_NYI();
    }

    void setTexture(std::string_view, Texture2D const&)
    {
        OSC_THROW_NYI();
    }

    glm::vec4 const* getVector(std::string_view) const
    {
        OSC_THROW_NYI();
    }

    void setVector(std::string_view, glm::vec4 const&)
    {
        OSC_THROW_NYI();
    }

    glm::mat4 const* getMatrix(std::string_view) const
    {
        OSC_THROW_NYI();
    }

    void setMatrix(std::string_view, glm::mat4 const&)
    {
        OSC_THROW_NYI();
    }

    std::size_t getHash() const
    {
        OSC_THROW_NYI();
    }
private:
    // TODO
};


class osc::experimental::Camera::Impl final {
public:
    Impl()
    {
        OSC_THROW_NYI();
    }

    explicit Impl(Texture2D)
    {
        OSC_THROW_NYI();
    }

    std::int64_t getVersion() const
    {
        OSC_THROW_NYI();
    }

    glm::vec4 const& getBackgroundColor() const
    {
        OSC_THROW_NYI();
    }

    void setBackgroundColor(glm::vec4 const&)
    {
        OSC_THROW_NYI();
    }

    CameraProjection getCameraProjection() const
    {
        OSC_THROW_NYI();
    }

    void setCameraProjection(CameraProjection)
    {
        OSC_THROW_NYI();
    }

    float getOrthographicSize() const
    {
        OSC_THROW_NYI();
    }

    void setOrthographicSize(float)
    {
        OSC_THROW_NYI();
    }

    float getCameraFOV() const
    {
        OSC_THROW_NYI();
    }

    void setCameraFOV(float)
    {
        OSC_THROW_NYI();
    }

    float getNearClippingPlane() const
    {
        OSC_THROW_NYI();
    }

    void setNearClippingPlane(float)
    {
        OSC_THROW_NYI();
    }

    float getFarClippingPlane() const
    {
        OSC_THROW_NYI();
    }

    void setFarClippingPlane(float)
    {
        OSC_THROW_NYI();
    }

    Texture2D const* getTexture() const
    {
        OSC_THROW_NYI();
    }

    void setTexture(Texture2D const&)
    {
        OSC_THROW_NYI();
    }

    void setTexture()
    {
        OSC_THROW_NYI();
    }

    Rect const& getPixelRect() const
    {
        OSC_THROW_NYI();
    }

    void setPixelRect(Rect const&)
    {
        OSC_THROW_NYI();
    }

    int getPixelWidth() const
    {
        OSC_THROW_NYI();
    }

    int getPixelHeight() const
    {
        OSC_THROW_NYI();
    }

    float getAspectRatio() const
    {
        OSC_THROW_NYI();
    }

    std::optional<Rect> getScissorRect() const
    {
        OSC_THROW_NYI();
    }

    void setScissorRect(Rect const&)
    {
        OSC_THROW_NYI();
    }

    void setScissorRect()
    {
        OSC_THROW_NYI();
    }

    glm::vec3 const& getPosition() const
    {
        OSC_THROW_NYI();
    }

    void setPosition(glm::vec3 const&)
    {
        OSC_THROW_NYI();
    }

    glm::vec3 const& getDirection() const
    {
        OSC_THROW_NYI();
    }

    void setDirection(glm::vec3 const&)
    {
        OSC_THROW_NYI();
    }

    glm::mat4 const& getCameraToWorldMatrix() const
    {
        OSC_THROW_NYI();
    }

    void render()
    {
        OSC_THROW_NYI();
    }

    std::size_t getHash() const
    {
        OSC_THROW_NYI();
    }

private:
    // TODO
};


// PUBLIC API

// MeshTopography

std::ostream& osc::experimental::operator<<(std::ostream& o, MeshTopography t)
{
    return o << g_MeshTopographyStrings.at(static_cast<std::size_t>(t));
}

std::string osc::experimental::to_string(MeshTopography t)
{
    return std::string{g_MeshTopographyStrings.at(static_cast<std::size_t>(t))};
}


// Mesh

osc::experimental::Mesh::Mesh() :
    m_Impl{std::make_shared<Impl>()}
{
}

osc::experimental::Mesh::Mesh(MeshTopography t, nonstd::span<glm::vec3 const> verts) :
    m_Impl{std::make_shared<Impl>(std::move(t), std::move(verts))}
{
}

osc::experimental::Mesh::Mesh(Mesh const&) = default;
osc::experimental::Mesh::Mesh(Mesh&&) noexcept = default;
osc::experimental::Mesh& osc::experimental::Mesh::operator=(Mesh const&) = default;
osc::experimental::Mesh& osc::experimental::Mesh::operator=(Mesh&&) noexcept = default;
osc::experimental::Mesh::~Mesh() noexcept = default;

std::int64_t osc::experimental::Mesh::getVersion() const
{
    return m_Impl->getVersion();
}

osc::experimental::MeshTopography osc::experimental::Mesh::getTopography() const
{
    return m_Impl->getTopography();
}

void osc::experimental::Mesh::setTopography(MeshTopography mt)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setTopography(std::move(mt));
}

nonstd::span<glm::vec3 const> osc::experimental::Mesh::getVerts() const
{
    return m_Impl->getVerts();
}

void osc::experimental::Mesh::setVerts(nonstd::span<const glm::vec3> vs)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setVerts(std::move(vs));
}

nonstd::span<glm::vec3 const> osc::experimental::Mesh::getNormals() const
{
    return m_Impl->getNormals();
}

void osc::experimental::Mesh::setNormals(nonstd::span<const glm::vec3> vs)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setNormals(std::move(vs));
}

nonstd::span<glm::vec2 const> osc::experimental::Mesh::getTexCoords() const
{
    return m_Impl->getTexCoords();
}

void osc::experimental::Mesh::setTexCoords(nonstd::span<const glm::vec2> tc)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setTexCoords(std::move(tc));
}

int osc::experimental::Mesh::getNumIndices() const
{
    return m_Impl->getNumIndices();
}

std::vector<uint32_t> osc::experimental::Mesh::getIndices() const
{
    return m_Impl->getIndices();
}

void osc::experimental::Mesh::setIndices(nonstd::span<const uint16_t> vs)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setIndices(std::move(vs));
}

void osc::experimental::Mesh::setIndices(nonstd::span<const uint32_t> vs)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setIndices(std::move(vs));
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

bool osc::experimental::operator<=(Mesh const& a, Mesh const& b)
{
    return a.m_Impl <= b.m_Impl;
}

bool osc::experimental::operator>(Mesh const& a, Mesh const& b)
{
    return a.m_Impl > b.m_Impl;
}

bool osc::experimental::operator>=(Mesh const& a, Mesh const& b)
{
    return a.m_Impl >= b.m_Impl;
}

std::ostream& osc::experimental::operator<<(std::ostream& o, Mesh const& m)
{
    return o << "Mesh(nverts = " << m.m_Impl->getVerts().size() << ", nindices = " << m.m_Impl->getIndices().size() << ')';
}

std::string osc::experimental::to_string(Mesh const& m)
{
    return StreamToString(m);
}

std::size_t std::hash<osc::experimental::Mesh>::operator()(osc::experimental::Mesh const& mesh) const
{
    return mesh.m_Impl->getHash();
}


// TextureWrapMode

std::ostream& osc::experimental::operator<<(std::ostream& o, TextureWrapMode wm)
{
    return o << g_TextureWrapModeStrings.at(static_cast<std::size_t>(wm));
}

std::string osc::experimental::to_string(TextureWrapMode wm)
{
    return std::string{g_TextureWrapModeStrings.at(static_cast<std::size_t>(wm))};
}


// TextureFilterMode

std::ostream& osc::experimental::operator<<(std::ostream& o, TextureFilterMode fm)
{
    return o << g_TextureFilterModeStrings.at(static_cast<std::size_t>(fm));
}

std::string osc::experimental::to_string(TextureFilterMode fm)
{
    return std::string{g_TextureFilterModeStrings.at(static_cast<std::size_t>(fm))};
}


// Texture2D

osc::experimental::Texture2D::Texture2D(int width, int height, nonstd::span<Rgba32 const> pixels) :
    m_Impl{std::make_shared<Impl>(std::move(width), std::move(height), std::move(pixels))}
{
}

osc::experimental::Texture2D::Texture2D(int width, int height, nonstd::span<glm::vec4 const> pixels) :
    m_Impl{std::make_shared<Impl>(std::move(width), std::move(height), std::move(pixels))}
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

void osc::experimental::Texture2D::setWrapMode(TextureWrapMode wm)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setWrapMode(std::move(wm));
}

osc::experimental::TextureWrapMode osc::experimental::Texture2D::getWrapModeU() const
{
    return m_Impl->getWrapModeU();
}

void osc::experimental::Texture2D::setWrapModeU(TextureWrapMode wm)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setWrapModeU(std::move(wm));
}

osc::experimental::TextureWrapMode osc::experimental::Texture2D::getWrapModeV() const
{
    return m_Impl->getWrapModeV();
}

void osc::experimental::Texture2D::setWrapModeV(TextureWrapMode wm)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setWrapModeV(std::move(wm));
}

osc::experimental::TextureWrapMode osc::experimental::Texture2D::getWrapModeW() const
{
    return m_Impl->getWrapModeW();
}

void osc::experimental::Texture2D::setWrapModeW(TextureWrapMode wm)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setWrapModeW(std::move(wm));
}

osc::experimental::TextureFilterMode osc::experimental::Texture2D::getFilterMode() const
{
    return m_Impl->getFilterMode();
}

void osc::experimental::Texture2D::setFilterMode(TextureFilterMode fm)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setFilterMode(std::move(fm));
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

bool osc::experimental::operator<=(Texture2D const& a, Texture2D const& b)
{
    return a.m_Impl <= b.m_Impl;
}

bool osc::experimental::operator>(Texture2D const& a, Texture2D const& b)
{
    return a.m_Impl > b.m_Impl;
}

bool osc::experimental::operator>=(Texture2D const& a, Texture2D const& b)
{
    return a.m_Impl >= b.m_Impl;
}

std::ostream& osc::experimental::operator<<(std::ostream& o, Texture2D const& t)
{
    return o << "Texture2D(width = " << t.getWidth() << ", height = " << t.getHeight() << ')';
}

std::string osc::experimental::to_string(Texture2D const& t)
{
    return StreamToString(t);
}

std::size_t std::hash<osc::experimental::Texture2D>::operator()(osc::experimental::Texture2D const& t) const
{
    return t.m_Impl->getHash();
}


// ShaderType

std::ostream& osc::experimental::operator<<(std::ostream& o, ShaderType st)
{
    return o << g_ShaderTypeStrings.at(static_cast<std::size_t>(st));
}

std::string osc::experimental::to_string(ShaderType st)
{
    return std::string{g_ShaderTypeStrings.at(static_cast<std::size_t>(st))};
}


// Shader

osc::experimental::Shader::Shader(char const* vertexShader, char const* fragmentShader) :
    m_Impl{std::make_shared<Impl>(std::move(vertexShader), std::move(fragmentShader))}
{
}

osc::experimental::Shader::Shader(Shader const&) = default;
osc::experimental::Shader::Shader(Shader&&) noexcept = default;
osc::experimental::Shader& osc::experimental::Shader::operator=(Shader const&) = default;
osc::experimental::Shader& osc::experimental::Shader::operator=(Shader&&) noexcept = default;
osc::experimental::Shader::~Shader() noexcept = default;

std::optional<int> osc::experimental::Shader::findPropertyIndex(std::string_view propertyName) const
{
    return m_Impl->findPropertyIndex(std::move(propertyName));
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

bool osc::experimental::operator<=(Shader const& a, Shader const& b)
{
    return a.m_Impl <= b.m_Impl;
}

bool osc::experimental::operator>(Shader const& a, Shader const& b)
{
    return a.m_Impl > b.m_Impl;
}

bool osc::experimental::operator>=(Shader const& a, Shader const& b)
{
    return a.m_Impl >= b.m_Impl;
}

std::ostream& osc::experimental::operator<<(std::ostream& o, Shader const& s)
{
    return o << "Shader(name)";
}

std::string osc::experimental::to_string(Shader const& s)
{
    return StreamToString(s);
}

std::size_t std::hash<osc::experimental::Shader>::operator()(osc::experimental::Shader const& s) const
{
    return s.m_Impl->getHash();
}


// Material

osc::experimental::Material::Material(Shader shader) :
    m_Impl{std::make_shared<Impl>(std::move(shader))}
{
}

osc::experimental::Material::Material(Material const&) = default;
osc::experimental::Material::Material(Material&&) noexcept = default;
osc::experimental::Material& osc::experimental::Material::operator=(Material const&) = default;
osc::experimental::Material& osc::experimental::Material::operator=(Material&&) noexcept = default;
osc::experimental::Material::~Material() noexcept = default;

std::int64_t osc::experimental::Material::getVersion() const
{
    return m_Impl->getVersion();
}

osc::experimental::Shader const& osc::experimental::Material::getShader() const
{
    return m_Impl->getShader();
}

bool osc::experimental::Material::hasProperty(std::string_view propertyName) const
{
    return m_Impl->hasProperty(std::move(propertyName));
}

glm::vec4 const* osc::experimental::Material::getColor() const
{
    return m_Impl->getVector("Color");
}

void osc::experimental::Material::setColor(glm::vec4 const& v)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setVector("Color", v);
}

float const* osc::experimental::Material::getFloat(std::string_view propertyName) const
{
    return m_Impl->getFloat(std::move(propertyName));
}

void osc::experimental::Material::setFloat(std::string_view propertyName, float v)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setFloat(std::move(propertyName), std::move(v));
}

int const* osc::experimental::Material::getInt(std::string_view propertyName) const
{
    return m_Impl->getInt(std::move(propertyName));
}

void osc::experimental::Material::setInt(std::string_view propertyName, int v)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setInt(std::move(propertyName), std::move(v));
}

osc::experimental::Texture2D const* osc::experimental::Material::getTexture(std::string_view propertyName) const
{
    return m_Impl->getTexture(std::move(propertyName));
}

void osc::experimental::Material::setTexture(std::string_view propertyName, Texture2D const& t)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setTexture(std::move(propertyName), std::move(t));
}

glm::vec4 const* osc::experimental::Material::getVector(std::string_view propertyName) const
{
    return m_Impl->getVector(std::move(propertyName));
}

void osc::experimental::Material::setVector(std::string_view propertyName, glm::vec4 const& v)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setVector(std::move(propertyName), v);
}

glm::mat4 const* osc::experimental::Material::getMatrix(std::string_view propertyName) const
{
    return m_Impl->getMatrix(std::move(propertyName));
}

void osc::experimental::Material::setMatrix(std::string_view propertyName, glm::mat4 const& m)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setMatrix(std::move(propertyName), m);
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

bool osc::experimental::operator<=(Material const& a, Material const& b)
{
    return a.m_Impl <= b.m_Impl;
}

bool osc::experimental::operator>(Material const& a, Material const& b)
{
    return a.m_Impl > b.m_Impl;
}

bool osc::experimental::operator>=(Material const& a, Material const& b)
{
    return a.m_Impl >= b.m_Impl;
}

std::ostream& osc::experimental::operator<<(std::ostream& o, Material const&)
{
    return o << "Material()";
}

std::string osc::experimental::to_string(Material const& mat)
{
    return StreamToString(mat);
}

std::size_t std::hash<osc::experimental::Material>::operator()(osc::experimental::Material const& mat) const
{
    return mat.m_Impl->getHash();
}


// osc::MaterialPropertyBlock

osc::experimental::MaterialPropertyBlock::MaterialPropertyBlock() : m_Impl{std::make_shared<Impl>()}
{
}

osc::experimental::MaterialPropertyBlock::MaterialPropertyBlock(MaterialPropertyBlock const&) = default;
osc::experimental::MaterialPropertyBlock::MaterialPropertyBlock(MaterialPropertyBlock&&) noexcept = default;
osc::experimental::MaterialPropertyBlock& osc::experimental::MaterialPropertyBlock::operator=(MaterialPropertyBlock const&) = default;
osc::experimental::MaterialPropertyBlock& osc::experimental::MaterialPropertyBlock::operator=(MaterialPropertyBlock&&) noexcept = default;
osc::experimental::MaterialPropertyBlock::~MaterialPropertyBlock() noexcept = default;

std::int64_t osc::experimental::MaterialPropertyBlock::getVersion() const
{
    return m_Impl->getVersion();
}

void osc::experimental::MaterialPropertyBlock::clear()
{
    DoCopyOnWrite(m_Impl);
    m_Impl->clear();
}

bool osc::experimental::MaterialPropertyBlock::isEmpty() const
{
    return m_Impl->isEmpty();
}

bool osc::experimental::MaterialPropertyBlock::hasProperty(std::string_view propertyName) const
{
    return m_Impl->hasProperty(std::move(propertyName));
}

glm::vec4 const* osc::experimental::MaterialPropertyBlock::getColor() const
{
    return m_Impl->getVector("Color");
}

void osc::experimental::MaterialPropertyBlock::setColor(glm::vec4 const& c)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setVector("Color", c);
}

float const* osc::experimental::MaterialPropertyBlock::getFloat(std::string_view propertyName) const
{
    return m_Impl->getFloat(std::move(propertyName));
}

void osc::experimental::MaterialPropertyBlock::setFloat(std::string_view propertyName, float v)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setFloat(std::move(propertyName), std::move(v));
}

int const* osc::experimental::MaterialPropertyBlock::getInt(std::string_view propertyName) const
{
    return m_Impl->getInt(std::move(propertyName));
}

void osc::experimental::MaterialPropertyBlock::setInt(std::string_view propertyName, int v)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setInt(std::move(propertyName), std::move(v));
}

osc::experimental::Texture2D const* osc::experimental::MaterialPropertyBlock::getTexture(std::string_view propertyName) const
{
    return m_Impl->getTexture(std::move(propertyName));
}

void osc::experimental::MaterialPropertyBlock::setTexture(std::string_view propertyName, Texture2D const& t)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setTexture(std::move(propertyName), std::move(t));
}

glm::vec4 const* osc::experimental::MaterialPropertyBlock::getVector(std::string_view propertyName) const
{
    return m_Impl->getVector(std::move(propertyName));
}

void osc::experimental::MaterialPropertyBlock::setVector(std::string_view propertyName, glm::vec4 const& v)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setVector(propertyName, v);
}

glm::mat4 const* osc::experimental::MaterialPropertyBlock::getMatrix(std::string_view propertyName) const
{
    return m_Impl->getMatrix(std::move(propertyName));
}

void osc::experimental::MaterialPropertyBlock::setMatrix(std::string_view propertyName, glm::mat4 const& v)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setMatrix(std::move(propertyName), v);
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

bool osc::experimental::operator<=(MaterialPropertyBlock const& a, MaterialPropertyBlock const& b)
{
    return a.m_Impl <= b.m_Impl;
}

bool osc::experimental::operator>(MaterialPropertyBlock const& a, MaterialPropertyBlock const& b)
{
    return a.m_Impl > b.m_Impl;
}

bool osc::experimental::operator>=(MaterialPropertyBlock const& a, MaterialPropertyBlock const& b)
{
    return a.m_Impl >= b.m_Impl;
}

std::ostream& osc::experimental::operator<<(std::ostream& o, MaterialPropertyBlock const&)
{
    return o << "MaterialPropertyBlock()";
}

std::string osc::experimental::to_string(MaterialPropertyBlock const& m)
{
    return StreamToString(m);
}

std::size_t std::hash<osc::experimental::MaterialPropertyBlock>::operator()(osc::experimental::MaterialPropertyBlock const& m) const
{
    return m.m_Impl->getHash();
}


// CameraProjection

std::ostream& osc::experimental::operator<<(std::ostream& o, CameraProjection p)
{
    return o << g_CameraProjectionStrings.at(static_cast<std::size_t>(p));
}

std::string osc::experimental::to_string(CameraProjection p)
{
    return std::string{g_CameraProjectionStrings.at(static_cast<std::size_t>(p))};
}


// Camera

osc::experimental::Camera::Camera() :
    m_Impl{std::make_shared<Impl>()}
{
}

osc::experimental::Camera::Camera(Texture2D t) :
    m_Impl{std::make_shared<Impl>(std::move(t))}
{
}

osc::experimental::Camera::Camera(Camera const&) = default;
osc::experimental::Camera::Camera(Camera&&) noexcept = default;
osc::experimental::Camera& osc::experimental::Camera::operator=(Camera const&) = default;
osc::experimental::Camera& osc::experimental::Camera::operator=(Camera&&) noexcept = default;
osc::experimental::Camera::~Camera() noexcept = default;

std::int64_t osc::experimental::Camera::getVersion() const
{
    return m_Impl->getVersion();
}

glm::vec4 const& osc::experimental::Camera::getBackgroundColor() const
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

void osc::experimental::Camera::setCameraProjection(CameraProjection p)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setCameraProjection(std::move(p));
}

float osc::experimental::Camera::getOrthographicSize() const
{
    return m_Impl->getOrthographicSize();
}

void osc::experimental::Camera::setOrthographicSize(float v)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setOrthographicSize(std::move(v));
}

float osc::experimental::Camera::getCameraFOV() const
{
    return m_Impl->getCameraFOV();
}

void osc::experimental::Camera::setCameraFOV(float v)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setCameraFOV(std::move(v));
}

float osc::experimental::Camera::getNearClippingPlane() const
{
    return m_Impl->getNearClippingPlane();
}

void osc::experimental::Camera::setNearClippingPlane(float v)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setNearClippingPlane(std::move(v));
}

float osc::experimental::Camera::getFarClippingPlane() const
{
    return m_Impl->getFarClippingPlane();
}

void osc::experimental::Camera::setFarClippingPlane(float v)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setFarClippingPlane(std::move(v));
}

osc::experimental::Texture2D const* osc::experimental::Camera::getTexture() const
{
    return m_Impl->getTexture();
}

void osc::experimental::Camera::setTexture(Texture2D const& t)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setTexture(std::move(t));
}

void osc::experimental::Camera::setTexture()
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setTexture();
}

osc::Rect const& osc::experimental::Camera::getPixelRect() const
{
    return m_Impl->getPixelRect();
}

void osc::experimental::Camera::setPixelRect(Rect const& r)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setPixelRect(r);
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

void osc::experimental::Camera::setScissorRect(Rect const& r)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setScissorRect(r);
}

void osc::experimental::Camera::setScissorRect()
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setScissorRect();
}

glm::vec3 const& osc::experimental::Camera::getPosition() const
{
    return m_Impl->getPosition();
}

void osc::experimental::Camera::setPosition(glm::vec3 const& p)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setPosition(p);
}

glm::vec3 const& osc::experimental::Camera::getDirection() const
{
    return m_Impl->getDirection();
}

void osc::experimental::Camera::setDirection(glm::vec3 const& d)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setDirection(d);
}

glm::mat4 const& osc::experimental::Camera::getCameraToWorldMatrix() const
{
    return m_Impl->getCameraToWorldMatrix();
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

bool osc::experimental::operator<=(Camera const& a, Camera const& b)
{
    return a.m_Impl <= b.m_Impl;
}

bool osc::experimental::operator>(Camera const& a, Camera const& b)
{
    return a.m_Impl > b.m_Impl;
}

bool osc::experimental::operator>=(Camera const& a, Camera const& b)
{
    return a.m_Impl >= b.m_Impl;
}

std::ostream& osc::experimental::operator<<(std::ostream& o, Camera const&)
{
    return o << "Camera()";
}

std::string osc::experimental::to_string(Camera const& c)
{
    return StreamToString(c);
}

size_t std::hash<osc::experimental::Camera>::operator()(osc::experimental::Camera const& c) const
{
    return c.m_Impl->getHash();
}


// Graphics

void osc::experimental::Graphics::DrawMesh(Mesh,
                                           glm::vec3 const&,
                                           Material,
                                           Camera&,
                                           std::optional<MaterialPropertyBlock>)
{
    // - copy the data onto the camera's command buffer list
    // - deal with any batch flushing etc.
    OSC_THROW_NYI();
}

*/