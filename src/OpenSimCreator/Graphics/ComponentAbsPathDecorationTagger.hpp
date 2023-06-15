#pragma once

#include "OpenSimCreator/Utils/OpenSimHelpers.hpp"

#include <oscar/Graphics/SceneDecoration.hpp>

#include <string>

namespace OpenSim { class Component; }

namespace osc
{
	// functor class that sets a decoration's ID the the component's abs path
	class ComponentAbsPathDecorationTagger final {
	public:
		void operator()(OpenSim::Component const& component, SceneDecoration& decoration)
		{
			if (&component != m_LastComponent)
			{
				GetAbsolutePathString(component, m_ID);
				m_LastComponent = &component;
			}

			decoration.id = m_ID;
		}
	private:
		OpenSim::Component const* m_LastComponent;
		std::string m_ID;
	};
}