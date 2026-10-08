#pragma once

#include <functional>
#include <string>

namespace OpenSim { class Object; }
namespace OpenSim { class AbstractProperty; }

namespace osc
{
    // concrete encapsulation of an edit that can be applied to an object
    //
    // this is designed to be safe to copy around etc. because it will perform
    // runtime lookups before applying the change
    class ObjectPropertyEdit final {
    public:
        ObjectPropertyEdit(
            const OpenSim::AbstractProperty&,
            std::function<void(OpenSim::AbstractProperty&)>
        );

        ObjectPropertyEdit(
            const OpenSim::Object&,
            const OpenSim::AbstractProperty&,
            std::function<void(OpenSim::AbstractProperty&)>
        );

        const std::string& get_component_abs_path() const;  // empty if it's just a standalone object
        const std::string& get_property_name() const;
        void apply(OpenSim::AbstractProperty&);
        const std::function<void(OpenSim::AbstractProperty&)>& get_updater() const
        {
            return updater_;
        }

    private:
        std::string component_abs_path_;
        std::string property_name_;
        std::function<void(OpenSim::AbstractProperty&)> updater_;
    };
}
