#pragma once

#include "src/Tabs/Tab.hpp"
#include "src/Utils/CStringView.hpp"
#include "src/Utils/UID.hpp"

#include <SDL_events.h>

#include <filesystem>
#include <memory>
#include <vector>

namespace osc { class MainUIStateAPI; }

namespace osc
{
    class MeshImporterTab final : public Tab {
    public:
        explicit MeshImporterTab(std::weak_ptr<MainUIStateAPI>);
        MeshImporterTab(std::weak_ptr<MainUIStateAPI>, std::vector<std::filesystem::path>);
        MeshImporterTab(MeshImporterTab const&) = delete;
        MeshImporterTab(MeshImporterTab&&) noexcept;
        MeshImporterTab& operator=(MeshImporterTab const&) = delete;
        MeshImporterTab& operator=(MeshImporterTab&&) noexcept;
        ~MeshImporterTab() noexcept override;

    private:
        UID implGetID() const final;
        CStringView implGetName() const final;
        bool implIsUnsaved() const final;
        bool implTrySave() final;
        void implOnMount() final;
        void implOnUnmount() final;
        bool implOnEvent(SDL_Event const&) final;
        void implOnTick() final;
        void implOnDrawMainMenu() final;
        void implOnDraw() final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
