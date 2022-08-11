#pragma once

#include "src/Graphics/ShaderType.hpp"
#include "src/Utils/CStringView.hpp"

#include <iosfwd>
#include <memory>
#include <optional>
#include <string>

// note: implementation is in `Renderer.cpp`
namespace osc
{
    // a handle to a shader
    class Shader final {
    public:
        Shader(CStringView vertexShader, CStringView fragmentShader);  // throws on compile error
        Shader(CStringView vertexShader, CStringView geometryShader, CStringView fragmmentShader);  // throws on compile error
        Shader(Shader const&);
        Shader(Shader&&) noexcept;
        Shader& operator=(Shader const&);
        Shader& operator=(Shader&&) noexcept;
        ~Shader() noexcept;

        std::optional<int> findPropertyIndex(std::string const& propertyName) const;

        int getPropertyCount() const;
        std::string const& getPropertyName(int propertyIndex) const;
        ShaderType getPropertyType(int propertyIndex) const;

    private:
        friend class GraphicsBackend;
        friend bool operator==(Shader const&, Shader const&);
        friend bool operator!=(Shader const&, Shader const&);
        friend bool operator<(Shader const&, Shader const&);
        friend std::ostream& operator<<(std::ostream&, Shader const&);

        class Impl;
        std::shared_ptr<Impl> m_Impl;
    };

    bool operator==(Shader const&, Shader const&);
    bool operator!=(Shader const&, Shader const&);
    bool operator<(Shader const&, Shader const&);
    std::ostream& operator<<(std::ostream&, Shader const&);
}