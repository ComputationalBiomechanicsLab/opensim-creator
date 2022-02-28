#pragma once

#include <span-lite/nonstd/span.hpp>

#include <functional>
#include <memory>
#include <optional>

namespace OpenSim
{
    class Object;
    class AbstractProperty;
}

namespace osc
{
    class ObjectPropertiesEditor final {
    public:
        struct Response final {
            OpenSim::AbstractProperty const& prop;
            std::function<void(OpenSim::AbstractProperty&)> updater;

            Response(OpenSim::AbstractProperty const& _prop,
                     std::function<void(OpenSim::AbstractProperty&)> _updater) :
                prop{std::move(_prop)},
                updater{std::move(_updater)}
            {
            }
        };

        ObjectPropertiesEditor();
        ObjectPropertiesEditor(ObjectPropertiesEditor const&) = delete;
        ObjectPropertiesEditor(ObjectPropertiesEditor&&) noexcept;
        ObjectPropertiesEditor& operator=(ObjectPropertiesEditor const&) = delete;
        ObjectPropertiesEditor& operator=(ObjectPropertiesEditor&&) noexcept;
        ~ObjectPropertiesEditor() noexcept;

        // if the user tries to edit one of the Object's properties, returns a
        // response that indicates which property was edited and a function that
        // performs an equivalent mutation to the property
        std::optional<Response> draw(OpenSim::Object const&);

        // as above, but only edit properties with the specified indices
        std::optional<Response> draw(OpenSim::Object const&, nonstd::span<int const> indices);

        struct Impl;
    private:
        std::unique_ptr<Impl> m_Impl;
    };
}
