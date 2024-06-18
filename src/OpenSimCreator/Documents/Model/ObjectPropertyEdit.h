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

        const std::string& getComponentAbsPath() const;  // empty if it's just a standalone object
        const std::string& getPropertyName() const;
        void apply(OpenSim::AbstractProperty&);
        const std::function<void(OpenSim::AbstractProperty&)>& getUpdater() const
        {
            return m_Updater;
        }

    private:
        std::string m_ComponentAbsPath;
        std::string m_PropertyName;
        std::function<void(OpenSim::AbstractProperty&)> m_Updater;
    };
}
