#pragma once

#include "src/Graphics/RenderTexture.hpp"
#include "src/Graphics/Shader.hpp"
#include "src/Graphics/Texture2D.hpp"

#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <nonstd/span.hpp>

#include <iosfwd>
#include <memory>
#include <optional>
#include <string_view>

// note: implementation is in `GraphicsImplementation.cpp`
namespace osc
{
    class Material final {
    public:
        explicit Material(Shader);
        Material(Material const&);
        Material(Material&&) noexcept;
        Material& operator=(Material const&);
        Material& operator=(Material&&) noexcept;
        ~Material() noexcept;

        Shader const& getShader() const;

        std::optional<float> getFloat(std::string_view propertyName) const;
        void setFloat(std::string_view propertyName, float);

        std::optional<nonstd::span<float const>> getFloatArray(std::string_view propertyName) const;
        void setFloatArray(std::string_view propertyName, nonstd::span<float const>);

        std::optional<glm::vec2> getVec2(std::string_view propertyName) const;
        void setVec2(std::string_view propertyName, glm::vec2);

        std::optional<glm::vec3> getVec3(std::string_view propertyName) const;
        void setVec3(std::string_view propertyName, glm::vec3);

        std::optional<nonstd::span<glm::vec3 const>> getVec3Array(std::string_view propertyName) const;
        void setVec3Array(std::string_view propertyName, nonstd::span<glm::vec3 const>);

        std::optional<glm::vec4> getVec4(std::string_view propertyName) const;
        void setVec4(std::string_view propertyName, glm::vec4);

        std::optional<glm::mat3> getMat3(std::string_view propertyName) const;
        void setMat3(std::string_view propertyName, glm::mat3 const&);

        std::optional<glm::mat4> getMat4(std::string_view propertyName) const;
        void setMat4(std::string_view propertyName, glm::mat4 const&);

        std::optional<int> getInt(std::string_view propertyName) const;
        void setInt(std::string_view propertyName, int);

        std::optional<bool> getBool(std::string_view propertyName) const;
        void setBool(std::string_view propertyName, bool);

        std::optional<Texture2D> getTexture(std::string_view propertyName) const;
        void setTexture(std::string_view propertyName, Texture2D);

        std::optional<RenderTexture> getRenderTexture(std::string_view propertyName) const;
        void setRenderTexture(std::string_view propertyName, RenderTexture);
        void clearRenderTexture(std::string_view propertyName);

        bool getTransparent() const;
        void setTransparent(bool);

        bool getDepthTested() const;
        void setDepthTested(bool);

        bool getWireframeMode() const;
        void setWireframeMode(bool);

    private:
        friend class GraphicsBackend;
        friend bool operator==(Material const&, Material const&);
        friend bool operator!=(Material const&, Material const&);
        friend bool operator<(Material const&, Material const&);
        friend std::ostream& operator<<(std::ostream&, Material const&);

        class Impl;
        std::shared_ptr<Impl> m_Impl;
    };

    bool operator==(Material const&, Material const&);
    bool operator!=(Material const&, Material const&);
    bool operator<(Material const&, Material const&);
    std::ostream& operator<<(std::ostream&, Material const&);
}