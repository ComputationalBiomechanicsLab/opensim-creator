#pragma once

namespace osc
{
	// marker interface for "an OSC-specific custom OpenSim component"
	//
	// primarily used to group those custom components under well-labelled UI components
	// (e.g. "experimental", "don't use this if you plan on having a standard osim file", etc.)
	class ICustomComponent {
	protected:
		ICustomComponent() = default;
		ICustomComponent(ICustomComponent const&) = default;
		ICustomComponent(ICustomComponent&&) noexcept = default;
		ICustomComponent& operator=(ICustomComponent const&) = default;
		ICustomComponent& operator=(ICustomComponent&&) noexcept = default;
	public:
		virtual ~ICustomComponent() noexcept = default;
	};
}