#pragma once

#include "src/Tabs/Tab.hpp"
#include "src/Utils/CStringView.hpp"

#include <SDL_events.h>

#include <filesystem>
#include <vector>

namespace osc
{
	class TabHost;

	class MeshImporterTab final : public Tab {
	public:
		MeshImporterTab(TabHost*);
		MeshImporterTab(TabHost*, std::vector<std::filesystem::path>);
		MeshImporterTab(MeshImporterTab const&) = delete;
		MeshImporterTab(MeshImporterTab&&) noexcept;
		MeshImporterTab& operator=(MeshImporterTab const&) = delete;
		MeshImporterTab& operator=(MeshImporterTab&&) noexcept;
		~MeshImporterTab() noexcept override;

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