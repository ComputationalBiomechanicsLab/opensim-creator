#pragma once

#include "OpenSimCreator/Utils/OpenSimHelpers.hpp"

#include <oscar/Graphics/SceneDecoration.hpp>
#include <oscar/Graphics/SceneDecorationFlags.hpp>

namespace OpenSim { class Component; }

namespace osc
{
    class ComponentSceneDecorationFlagsTagger final {
    public:
        ComponentSceneDecorationFlagsTagger(
            OpenSim::Component const* selected_,
            OpenSim::Component const* hovered_) :
            m_Selected{selected_},
            m_Hovered{hovered_}
        {
        }

        void operator()(OpenSim::Component const& component, SceneDecoration& decoration)
        {
            if (&component != m_LastComponent)
            {
                m_Flags = ComputeFlags(component);
                m_LastComponent = &component;
            }

            decoration.flags = m_Flags;
        }
    private:
        SceneDecorationFlags ComputeFlags(OpenSim::Component const& component) const
        {
            SceneDecorationFlags rv = SceneDecorationFlags_CastsShadows;

            if (&component == m_Selected)
            {
                rv |= SceneDecorationFlags_IsSelected;
            }

            if (&component == m_Hovered)
            {
                rv |= SceneDecorationFlags_IsHovered;
            }

            OpenSim::Component const* ptr = GetOwner(component);
            while (ptr)
            {
                if (ptr == m_Selected)
                {
                    rv |= SceneDecorationFlags_IsChildOfSelected;
                }
                if (ptr == m_Hovered)
                {
                    rv |= SceneDecorationFlags_IsChildOfHovered;
                }
                ptr = GetOwner(*ptr);
            }

            return rv;
        }

        OpenSim::Component const* m_Selected;
        OpenSim::Component const* m_Hovered;
        OpenSim::Component const* m_LastComponent = nullptr;
        SceneDecorationFlags m_Flags = SceneDecorationFlags_None;
    };
}