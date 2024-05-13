#pragma once

#include <oscar/UI/Tabs/ITab.h>
#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/UID.h>

#include <memory>

namespace osc { template<typename T> class ParentPtr; }
namespace osc { class ITabHost; }

namespace osc
{
    class MeshGenTestTab final : public ITab {
    public:
        static CStringView id();

        explicit MeshGenTestTab(const ParentPtr<ITabHost>&);
        MeshGenTestTab(const MeshGenTestTab&) = delete;
        MeshGenTestTab(MeshGenTestTab&&) noexcept;
        MeshGenTestTab& operator=(const MeshGenTestTab&) = delete;
        MeshGenTestTab& operator=(MeshGenTestTab&&) noexcept;
        ~MeshGenTestTab() noexcept override;

    private:
        UID impl_get_id() const final;
        CStringView impl_get_name() const final;
        void impl_on_draw() final;

        class Impl;
        std::unique_ptr<Impl> impl_;
    };
}
