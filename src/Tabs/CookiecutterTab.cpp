#include "CookiecutterTab.hpp"

class osc::CookiecutterTab::Impl final {
public:
	Impl(TabHost* parent) : m_Parent{std::move(parent)}
	{
	}

	void onMount()
	{

	}

	void onUnmount()
	{

	}

	bool onEvent(SDL_Event const&)
	{
		return true;
	}

	void onTick()
	{

	}

	void drawMainMenu()
	{

	}
	void onDraw()
	{

	}

	CStringView name()
	{
		return m_Name;
	}

	TabHost* parent()
	{
		return m_Parent;
	}

private:
	std::string m_Name = "CookiecutterTab";
	TabHost* m_Parent;
};


// public API

osc::CookiecutterTab::CookiecutterTab(TabHost* parent) :
	m_Impl{new Impl{std::move(parent)}}
{
}

osc::CookiecutterTab::CookiecutterTab(CookiecutterTab&& tmp) noexcept :
	m_Impl{std::exchange(tmp.m_Impl, nullptr)}
{
}

osc::CookiecutterTab& osc::CookiecutterTab::operator=(CookiecutterTab&& tmp) noexcept
{
	std::swap(m_Impl, tmp.m_Impl);
	return *this;
}

osc::CookiecutterTab::~CookiecutterTab() noexcept
{
	delete m_Impl;
}

void osc::CookiecutterTab::implOnMount()
{
	m_Impl->onMount();
}

void osc::CookiecutterTab::implOnUnmount()
{
	m_Impl->onUnmount();
}

bool osc::CookiecutterTab::implOnEvent(SDL_Event const& e)
{
	return m_Impl->onEvent(e);
}

void osc::CookiecutterTab::implOnTick()
{
	m_Impl->onTick();
}

void osc::CookiecutterTab::implDrawMainMenu()
{
	m_Impl->drawMainMenu();
}

void osc::CookiecutterTab::implOnDraw()
{
	m_Impl->onDraw();
}

osc::CStringView osc::CookiecutterTab::implName()
{
	return m_Impl->name();
}

osc::TabHost* osc::CookiecutterTab::implParent()
{
	return m_Impl->parent();
}