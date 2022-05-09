#include "ComponentAnnotations.hpp"

#include "src/OpenSimBindings/OpenSimHelpers.hpp"

#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentPath.h>

#include <utility>

class osc::ComponentAnnotations::Impl final {
public:

	OpenSim::ComponentPath const& getSelectedPath() const
	{
		return m_SelectionPath;
	}

	OpenSim::Component const* getSelectedPtr(OpenSim::Component const& root) const
	{
		return FindComponent(root, m_SelectionPath);
	}

	OpenSim::Component* updSelectedPtr(OpenSim::Component& root) const
	{
		return FindComponentMut(root, m_SelectionPath);
	}

	void setSelected()
	{
		m_SelectionPath = OpenSim::ComponentPath{};
	}

	void setSelected(OpenSim::Component const& c)
	{
		m_SelectionPath = c.getAbsolutePath();
	}

	void setSelected(OpenSim::ComponentPath const& cp)
	{
		m_SelectionPath = cp;
	}

	OpenSim::ComponentPath const& getHovererdPath() const
	{
		return m_HoverPath;
	}

	OpenSim::Component const* getHoveredPtr(OpenSim::Component const& root) const
	{
		return FindComponent(root, m_HoverPath);
	}

	OpenSim::Component* updHoveredPtr(OpenSim::Component& root) const
	{
		return FindComponentMut(root, m_HoverPath);
	}

	void setHovered()
	{
		m_HoverPath = OpenSim::ComponentPath{};
	}

	void setHovered(OpenSim::Component const& c)
	{
		m_HoverPath = c.getAbsolutePath();
	}

	void setHovered(OpenSim::ComponentPath const& cp)
	{
		m_HoverPath = cp;
	}

	OpenSim::ComponentPath const& getIsolatedPath() const
	{
		return m_IsolatedPath;
	}

	OpenSim::Component const* getIsolatedPtr(OpenSim::Component const& root) const
	{
		return FindComponent(root, m_IsolatedPath);
	}

	OpenSim::Component* updIsolatedPtr(OpenSim::Component& root) const
	{
		return FindComponentMut(root, m_IsolatedPath);
	}

	void setIsolated()
	{
		m_IsolatedPath = OpenSim::ComponentPath{};
	}

	void setIsolated(OpenSim::Component const& c)
	{
		m_IsolatedPath = OpenSim::ComponentPath{};
	}

	void setIsolated(OpenSim::ComponentPath const& cp)
	{
		m_IsolatedPath = cp;
	}

	float getFixupScaleFactor() const
	{
		return m_FixupScaleFactor;
	}

	void setFixupScaleFactor(float fs)
	{
		m_FixupScaleFactor = std::move(fs);
	}

private:
	OpenSim::ComponentPath m_SelectionPath;
	OpenSim::ComponentPath m_HoverPath;
	OpenSim::ComponentPath m_IsolatedPath;
	float m_FixupScaleFactor = 1.0f;
};


// public API (PIMPL)

osc::ComponentAnnotations::ComponentAnnotations() :
	m_Impl{new Impl{}}
{
}

osc::ComponentAnnotations::ComponentAnnotations(ComponentAnnotations const& other) :
	m_Impl{new Impl{*other.m_Impl}}
{
}

osc::ComponentAnnotations::ComponentAnnotations(ComponentAnnotations&& tmp) noexcept :
	m_Impl{std::exchange(tmp.m_Impl, nullptr)}
{
}

osc::ComponentAnnotations& osc::ComponentAnnotations::operator=(ComponentAnnotations const& other)
{
	if (&other != this)
	{
		*m_Impl = *other.m_Impl;
	}
	return *this;
}

osc::ComponentAnnotations& osc::ComponentAnnotations::operator=(ComponentAnnotations&& tmp) noexcept
{
	std::swap(m_Impl, tmp.m_Impl);
	return *this;
}

osc::ComponentAnnotations::~ComponentAnnotations() noexcept
{
	delete m_Impl;
}

OpenSim::ComponentPath const& osc::ComponentAnnotations::getSelectedPath() const
{
	return m_Impl->getSelectedPath();
}

OpenSim::Component const* osc::ComponentAnnotations::getSelectedPtr(OpenSim::Component const& root) const
{
	return m_Impl->getSelectedPtr(root);
}

OpenSim::Component* osc::ComponentAnnotations::updSelectedPtr(OpenSim::Component& root) const
{
	return m_Impl->updSelectedPtr(root);
}

void osc::ComponentAnnotations::setSelected()
{
	m_Impl->setSelected();
}

void osc::ComponentAnnotations::setSelected(OpenSim::Component const& c)
{
	m_Impl->setSelected(c);
}

void osc::ComponentAnnotations::setSelected(OpenSim::ComponentPath const& cp)
{
	m_Impl->setSelected(cp);
}

OpenSim::ComponentPath const& osc::ComponentAnnotations::getHovererdPath() const
{
	return m_Impl->getHovererdPath();
}

OpenSim::Component const* osc::ComponentAnnotations::getHoveredPtr(OpenSim::Component const& root) const
{
	return m_Impl->getHoveredPtr(root);
}

OpenSim::Component* osc::ComponentAnnotations::updHoveredPtr(OpenSim::Component& root) const
{
	return m_Impl->updHoveredPtr(root);
}

void osc::ComponentAnnotations::setHovered()
{
	m_Impl->setHovered();
}

void osc::ComponentAnnotations::setHovered(OpenSim::Component const& c)
{
	m_Impl->setHovered(c);
}

void osc::ComponentAnnotations::setHovered(OpenSim::ComponentPath const& cp)
{
	m_Impl->setHovered(cp);
}

OpenSim::ComponentPath const& osc::ComponentAnnotations::getIsolatedPath() const
{
	return m_Impl->getIsolatedPath();
}

OpenSim::Component const* osc::ComponentAnnotations::getIsolatedPtr(OpenSim::Component const& root) const
{
	return m_Impl->getIsolatedPtr(root);
}

OpenSim::Component* osc::ComponentAnnotations::updIsolatedPtr(OpenSim::Component& root) const
{
	return m_Impl->updIsolatedPtr(root);
}

void osc::ComponentAnnotations::setIsolated()
{
	m_Impl->setIsolated();
}

void osc::ComponentAnnotations::setIsolated(OpenSim::Component const& c)
{
	m_Impl->setIsolated(c);
}

void osc::ComponentAnnotations::setIsolated(OpenSim::ComponentPath const& cp)
{
	m_Impl->setIsolated(cp);
}

float osc::ComponentAnnotations::getFixupScaleFactor() const
{
	return m_Impl->getFixupScaleFactor();
}

void osc::ComponentAnnotations::setFixupScaleFactor(float fs)
{
	m_Impl->setFixupScaleFactor(std::move(fs));
}