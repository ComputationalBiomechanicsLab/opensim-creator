#pragma once

#include <SDL_events.h>
#include <oscar/UI/Tabs/ITab.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/UID.hpp>

#include <filesystem>
#include <memory>
#include <vector>

namespace osc { class IMainUIStateAPI; }
namespace osc { template<typename T> class ParentPtr; }

namespace osc::mi
{
    class MeshImporterTab final : public ITab {
    public:
        explicit MeshImporterTab(ParentPtr<IMainUIStateAPI> const&);
        MeshImporterTab(ParentPtr<IMainUIStateAPI> const&, std::vector<std::filesystem::path>);
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
