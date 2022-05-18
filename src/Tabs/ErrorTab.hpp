#pragma once

#include "src/Tabs/Tab.hpp"
#include "src/Utils/CStringView.hpp"
#include "src/Utils/UID.hpp"

#include <SDL_events.h>

#include <stdexcept>

namespace osc
{
	class TabHost;

	class ErrorTab final : public Tab {
	public:
		ErrorTab(TabHost*, std::exception const&);
		ErrorTab(ErrorTab const&) = delete;
		ErrorTab(ErrorTab&&) noexcept;
		ErrorTab& operator=(ErrorTab const&) = delete;
		ErrorTab& operator=(ErrorTab&&) noexcept;
		~ErrorTab() noexcept override;

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