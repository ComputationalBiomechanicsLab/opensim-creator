#pragma once

#include "src/Tabs/Tab.hpp"
#include "src/Utils/CStringView.hpp"
#include "src/Utils/UID.hpp"

#include <SDL_events.h>

namespace osc
{
	class TabHost;
}

namespace osc
{
	class CookiecutterTab final : public Tab {
	public:
		CookiecutterTab(TabHost*);
		CookiecutterTab(CookiecutterTab const&) = delete;
		CookiecutterTab(CookiecutterTab&&) noexcept;
		CookiecutterTab& operator=(CookiecutterTab const&) = delete;
		CookiecutterTab& operator=(CookiecutterTab&&) noexcept;
		~CookiecutterTab() noexcept override;

	private:
		UID implGetID() const override;
		CStringView implGetName() const override;
		TabHost* implParent() const override;
		void implOnMount() override;
		void implOnUnmount() override;
		bool implOnEvent(SDL_Event const&) override;
		void implOnTick() override;
		void implOnDrawMainMenu() override;
		void implOnDraw() override;

		class Impl;
		Impl* m_Impl;
	};
}