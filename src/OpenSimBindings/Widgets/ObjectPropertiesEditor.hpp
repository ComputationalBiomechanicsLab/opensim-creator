#pragma once

#include <nonstd/span.hpp>

#include <functional>
#include <memory>
#include <optional>
#include <string>

namespace OpenSim { class Object; }
namespace OpenSim { class AbstractProperty; }

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

    private:
        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
