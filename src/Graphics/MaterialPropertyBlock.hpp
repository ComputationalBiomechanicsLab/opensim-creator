#pragma once

#include "src/Graphics/Texture2D.hpp"

#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <iosfwd>
#include <memory>
#include <optional>
#include <string_view>

// note: implementation is in `Renderer.cpp`
namespace osc
{
    // material property block
    //
    // enables callers to apply per-instance properties when using a material (more efficiently
    // than using a different material every time)
    class MaterialPropertyBlock final {
    public:
        MaterialPropertyBlock();
        MaterialPropertyBlock(MaterialPropertyBlock const&);
        MaterialPropertyBlock(MaterialPropertyBlock&&) noexcept;
        MaterialPropertyBlock& operator=(MaterialPropertyBlock const&);
        MaterialPropertyBlock& operator=(MaterialPropertyBlock&&) noexcept;
        ~MaterialPropertyBlock() noexcept;

        void clear();
        bool isEmpty() const;

        std::optional<float> getFloat(std::string_view propertyName) const;
        void setFloat(std::string_view propertyName, float);

        std::optional<glm::vec3> getVec3(std::string_view propertyName) const;
        void setVec3(std::string_view propertyName, glm::vec3);

        std::optional<glm::vec4> getVec4(std::string_view propertyName) const;
        void setVec4(std::string_view propertyName, glm::vec4);

        std::optional<glm::mat3> getMat3(std::string_view propertyName) const;
        void setMat3(std::string_view propertyName, glm::mat3 const&);

        std::optional<glm::mat4> getMat4(std::string_view propertyName) const;
        void setMat4(std::string_view propertyName, glm::mat4 const&);

        std::optional<int> getInt(std::string_view propertyName) const;
        void setInt(std::string_view, int);

        std::optional<bool> getBool(std::string_view propertyName) const;
        void setBool(std::string_view propertyName, bool);

        std::optional<Texture2D> getTexture(std::string_view propertyName) const;
        void setTexture(std::string_view, Texture2D);

    private:
        friend class GraphicsBackend;
        friend bool operator==(MaterialPropertyBlock const&, MaterialPropertyBlock const&);
        friend bool operator!=(MaterialPropertyBlock const&, MaterialPropertyBlock const&);
        friend bool operator<(MaterialPropertyBlock const&, MaterialPropertyBlock const&);
        friend std::ostream& operator<<(std::ostream&, MaterialPropertyBlock const&);

        class Impl;
        std::shared_ptr<Impl> m_Impl;
    };

    bool operator==(MaterialPropertyBlock const&, MaterialPropertyBlock const&);
    bool operator!=(MaterialPropertyBlock const&, MaterialPropertyBlock const&);
    bool operator<(MaterialPropertyBlock const&, MaterialPropertyBlock const&);
    std::ostream& operator<<(std::ostream&, MaterialPropertyBlock const&);
}