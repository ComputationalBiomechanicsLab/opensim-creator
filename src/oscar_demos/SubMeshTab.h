#pragma once

#include <oscar/UI/Tabs/ITab.h>
#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/UID.h>

#include <memory>

namespace osc { template<typename T> class ParentPtr; }
namespace osc { class ITabHost; }

namespace osc
{
    class SubMeshTab final : public ITab {
    public:
        static CStringView id();

        explicit SubMeshTab(ParentPtr<ITabHost> const&);
        SubMeshTab(SubMeshTab const&) = delete;
        SubMeshTab(SubMeshTab&&) noexcept;
        SubMeshTab& operator=(SubMeshTab const&) = delete;
        SubMeshTab& operator=(SubMeshTab&&) noexcept;
        ~SubMeshTab() noexcept override;

    private:
        UID implGetID() const final;
        CStringView implGetName() const final;
        void implOnDraw() final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
