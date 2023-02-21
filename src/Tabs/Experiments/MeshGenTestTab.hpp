#pragma once

#include "src/Tabs/Tab.hpp"
#include "src/Utils/CStringView.hpp"
#include "src/Utils/UID.hpp"

#include <memory>

namespace osc { class TabHost; }

namespace osc
{
    class MeshGenTestTab final : public Tab {
    public:
        static CStringView id() noexcept;

        explicit MeshGenTestTab(std::weak_ptr<TabHost>);
        MeshGenTestTab(MeshGenTestTab const&) = delete;
        MeshGenTestTab(MeshGenTestTab&&) noexcept;
        MeshGenTestTab& operator=(MeshGenTestTab const&) = delete;
        MeshGenTestTab& operator=(MeshGenTestTab&&) noexcept;
        ~MeshGenTestTab() noexcept override;

    private:
        UID implGetID() const final;
        CStringView implGetName() const final;
        void implOnDraw() final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}