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
            OpenSim::AbstractProperty const&,
            std::function<void(OpenSim::AbstractProperty&)>
        );

        ObjectPropertyEdit(
            OpenSim::Object const&,
            OpenSim::AbstractProperty const&,
            std::function<void(OpenSim::AbstractProperty&)>
        );

        std::string const& getComponentAbsPath() const;  // empty if it's just a standalone object
        std::string const& getPropertyName() const;
        void apply(OpenSim::AbstractProperty&);
        std::function<void(OpenSim::AbstractProperty&)> const& getUpdater() const
        {
            return m_Updater;
        }

    private:
        std::string m_ComponentAbsPath;
        std::string m_PropertyName;
        std::function<void(OpenSim::AbstractProperty&)> m_Updater;
    };
}