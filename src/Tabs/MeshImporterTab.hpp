#pragma once

#include "src/Tabs/Tab.hpp"
#include "src/Utils/CStringView.hpp"
#include "src/Utils/UID.hpp"

#include <SDL_events.h>

#include <filesystem>
#include <vector>

namespace osc
{
	class MainUIStateAPI;
    class TabHost;
}

namespace osc
{
	class MeshImporterTab final : public Tab {
	public:
        explicit MeshImporterTab(MainUIStateAPI*);
		MeshImporterTab(MainUIStateAPI*, std::vector<std::filesystem::path>);
		MeshImporterTab(MeshImporterTab const&) = delete;
		MeshImporterTab(MeshImporterTab&&) noexcept;
		MeshImporterTab& operator=(MeshImporterTab const&) = delete;
		MeshImporterTab& operator=(MeshImporterTab&&) noexcept;
		~MeshImporterTab() noexcept override;

	private:
		UID implGetID() const override;
		CStringView implGetName() const override;
		TabHost* implParent() const override;
		bool implIsUnsaved() const override;
		bool implTrySave() override;
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
