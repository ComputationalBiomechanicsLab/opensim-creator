#pragma once

#include <oscar/UI/Tabs/ITab.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/UID.hpp>

#include <memory>

namespace osc { class IMainUIStateAPI; }
namespace osc { template<typename T> class ParentPtr; }
namespace osc { class Simulation; }

namespace osc
{
    class SimulatorTab final : public ITab {
    public:
        SimulatorTab(
            ParentPtr<IMainUIStateAPI> const&,
            std::shared_ptr<Simulation>
        );
        SimulatorTab(SimulatorTab const&) = delete;
        SimulatorTab(SimulatorTab&&) noexcept;
        SimulatorTab& operator=(SimulatorTab const&) = delete;
        SimulatorTab& operator=(SimulatorTab&&) noexcept;
        ~SimulatorTab() noexcept override;

    private:
        UID implGetID() const final;
        CStringView implGetName() const final;
        void implOnMount() final;
        void implOnUnmount() final;
        void implOnTick() final;
        void implOnDrawMainMenu() final;
        void implOnDraw() final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
