#pragma once

#include <span-lite/nonstd/span.hpp>

#include <functional>
#include <optional>
#include <string>

namespace OpenSim
{
    class Object;
    class AbstractProperty;
}

namespace osc
{
    class ObjectPropertyEdit final {
    public:
        ObjectPropertyEdit(
            OpenSim::Object const&,
            OpenSim::AbstractProperty const&,
            std::function<void(OpenSim::AbstractProperty&)>);

        std::string const& getComponentAbsPath() const;  // empty if it's just a standalone object
        std::string const& getPropertyName() const;
        void apply(OpenSim::AbstractProperty&);

    private:
        std::string m_ComponentAbsPath;
        std::string m_PropertyName;
        std::function<void(OpenSim::AbstractProperty&)> m_Updater;
    };

    class ObjectPropertiesEditor final {
    public:
        ObjectPropertiesEditor();
        ObjectPropertiesEditor(ObjectPropertiesEditor const&) = delete;
        ObjectPropertiesEditor(ObjectPropertiesEditor&&) noexcept;
        ObjectPropertiesEditor& operator=(ObjectPropertiesEditor const&) = delete;
        ObjectPropertiesEditor& operator=(ObjectPropertiesEditor&&) noexcept;
        ~ObjectPropertiesEditor() noexcept;

        // if the user tries to edit an Object's properties, returns a response that lets
        // callers apply the edit
        std::optional<ObjectPropertyEdit> draw(OpenSim::Object const&);

        // if the user tries to edit an Object's properties, returns a response that lets
        // callers apply the edit
        std::optional<ObjectPropertyEdit> draw(OpenSim::Object const&, nonstd::span<int const> indices);

    private:
        class Impl;
        Impl* m_Impl;
    };
}
