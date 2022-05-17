#pragma once

#include "src/Tabs/Tab.hpp"
#include "src/Utils/CStringView.hpp"

#include <SDL_events.h>

namespace osc
{
	class TabHost;

	class CookiecutterTab final : public Tab {
	public:
		CookiecutterTab(TabHost*);
		CookiecutterTab(CookiecutterTab const&) = delete;
		CookiecutterTab(CookiecutterTab&&) noexcept;
		CookiecutterTab& operator=(CookiecutterTab const&) = delete;
		CookiecutterTab& operator=(CookiecutterTab&&) noexcept;
		~CookiecutterTab() noexcept override;

	private:
		void implOnMount() override;
		void implOnUnmount() override;
		bool implOnEvent(SDL_Event const&) override;
		void implOnTick() override;
		void implDrawMainMenu() override;
		void implOnDraw() override;
		CStringView implName() override;
		TabHost* implParent() override;

		class Impl;
		Impl* m_Impl;
	};
}