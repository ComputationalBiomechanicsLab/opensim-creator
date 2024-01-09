#pragma once

#include <oscar/UI/Tabs/ITab.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/UID.hpp>

#include <memory>

namespace osc { template<typename T> class ParentPtr; }
namespace osc { class ITabHost; }

namespace osc
{
    class MeshGenTestTab final : public ITab {
    public:
        static CStringView id();

        explicit MeshGenTestTab(ParentPtr<ITabHost> const&);
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
