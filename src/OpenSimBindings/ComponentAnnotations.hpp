#pragma once

namespace OpenSim
{
	class Component;
	class ComponentPath;
}

namespace osc
{
	// a value type that holds annotations for some root (usually, model) component
	//
	// "annotations" include:
	// 
	// - selection
	// - hover
	// - isolation
	// - 3D scaling fixups
	//
	// annotations are held separately from the model+state because they can vary somewhat
	// independently of them (e.g. a selection can be "ported" to a new model+state)
	class ComponentAnnotations final {
	public:
		ComponentAnnotations();
		ComponentAnnotations(ComponentAnnotations const&);
		ComponentAnnotations(ComponentAnnotations&&) noexcept;
		ComponentAnnotations& operator=(ComponentAnnotations const&);
		ComponentAnnotations& operator=(ComponentAnnotations&&) noexcept;
		~ComponentAnnotations() noexcept;

		OpenSim::ComponentPath const& getSelectedPath() const;
		OpenSim::Component const* getSelectedPtr(OpenSim::Component const& root) const;
		template<typename T>
		T const* getSelectedPtrAs(OpenSim::Component const& root) const
		{
			return dynamic_cast<T const*>(getSelectedPtr(root));
		}
		OpenSim::Component* updSelectedPtr(OpenSim::Component& root) const;
		template<typename T>
		T* updSelectedPtrAs(OpenSim::Component& root) const
		{
			return dynamic_cast<T*>(updSelectedPtr(root));
		}
		void setSelected();
		void setSelected(OpenSim::Component const&);
		void setSelected(OpenSim::ComponentPath const&);

		OpenSim::ComponentPath const& getHovererdPath() const;
		OpenSim::Component const* getHoveredPtr(OpenSim::Component const& root) const;
		template<typename T>
		T const* getHoveredPtrAs(OpenSim::Component const& root) const
		{
			return dynamic_cast<T const*>(getHoveredPtr(root));
		}
		OpenSim::Component* updHoveredPtr(OpenSim::Component& root) const;
		template<typename T>
		T* updHoveredPtrAs(OpenSim::Component& root) const
		{
			return dynamic_cast<T*>(updHoveredPtr(root));
		}
		void setHovered();
		void setHovered(OpenSim::Component const&);
		void setHovered(OpenSim::ComponentPath const&);

		OpenSim::ComponentPath const& getIsolatedPath() const;
		OpenSim::Component const* getIsolatedPtr(OpenSim::Component const& root) const;
		template<typename T>
		T const* getIsolatedPtrAs(OpenSim::Component const& root) const
		{
			return dynamic_cast<T const*>(getIsolatedPtr(root));
		}
		OpenSim::Component* updIsolatedPtr(OpenSim::Component& root) const;
		template<typename T>
		T* updIsolatedPtrAs(OpenSim::Component& root) const
		{
			return dynamic_cast<T*>(updIsolatedPtr(root));
		}
		void setIsolated();
		void setIsolated(OpenSim::Component const&);
		void setIsolated(OpenSim::ComponentPath const&);

		float getFixupScaleFactor() const;
		void setFixupScaleFactor(float);

		class Impl;
	private:
		Impl* m_Impl;
	};
}