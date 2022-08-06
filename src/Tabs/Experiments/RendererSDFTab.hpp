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
	class RendererSDFTab final : public Tab {
	public:
		RendererSDFTab(TabHost*);
		RendererSDFTab(RendererSDFTab const&) = delete;
		RendererSDFTab(RendererSDFTab&&) noexcept;
		RendererSDFTab& operator=(RendererSDFTab const&) = delete;
		RendererSDFTab& operator=(RendererSDFTab&&) noexcept;
		~RendererSDFTab() noexcept override;

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