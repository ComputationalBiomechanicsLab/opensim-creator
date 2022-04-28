#pragma once

#include "src/OpenSimBindings/VirtualConstModelStatePair.hpp"
#include "src/Utils/UID.hpp"

#include <optional>

namespace OpenSim
{
	class Component;
	class Model;	
}

namespace SimTK
{
	class State;
}

namespace osc
{
	// virtual read+write accessor to an `OpenSim::Model` + `SimTK::State` pair, with
	// additional opt-in overrides to aid rendering/UX etc.
	class VirtualModelStatePair : public VirtualConstModelStatePair {
	public:
		virtual ~VirtualModelStatePair() noexcept = default;

		virtual OpenSim::Model& updModel() = 0;

		virtual OpenSim::Component* updSelected() { return nullptr; }
		virtual void setSelected(OpenSim::Component const*) {}

		virtual OpenSim::Component* updHovered() { return nullptr; }
		virtual void setHovered(OpenSim::Component const*) {}

		virtual OpenSim::Component* updIsolated() { return nullptr; }
		virtual void setIsolated(OpenSim::Component const*) {}

		virtual void setFixupScaleFactor(float) {}

		// concrete helper methods that use the above virutal API

		template<typename T>
		T* updSelectedAs()
		{
			return dynamic_cast<T*>(updSelected());
		}

		virtual void setSelectedHoveredAndIsolatedFrom(VirtualModelStatePair const& other)
		{
			setSelected(other.getSelected());
			setHovered(other.getHovered());
			setIsolated(other.getIsolated());
		}
	};
}