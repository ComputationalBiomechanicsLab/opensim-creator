#pragma once

#include "src/Tabs/Tab.hpp"
#include "src/Utils/CStringView.hpp"

#include <SDL_events.h>

namespace osc
{
	class TabHost;

	class SplashTab final : public Tab {
	public:
		SplashTab(TabHost*);
		SplashTab(SplashTab const&) = delete;
		SplashTab(SplashTab&&) noexcept;
		SplashTab& operator=(SplashTab const&) = delete;
		SplashTab& operator=(SplashTab&&) noexcept;
		~SplashTab() noexcept override;

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