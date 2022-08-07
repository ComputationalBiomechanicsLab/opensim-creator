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
	class MeshGenTestTab final : public Tab {
	public:
		MeshGenTestTab(TabHost*);
		MeshGenTestTab(MeshGenTestTab const&) = delete;
		MeshGenTestTab(MeshGenTestTab&&) noexcept;
		MeshGenTestTab& operator=(MeshGenTestTab const&) = delete;
		MeshGenTestTab& operator=(MeshGenTestTab&&) noexcept;
		~MeshGenTestTab() noexcept override;

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