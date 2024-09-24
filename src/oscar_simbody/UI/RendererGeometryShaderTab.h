#pragma once

#include <oscar/UI/Tabs/ITab.h>
#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/UID.h>

#include <memory>

namespace osc { template<typename T> class ParentPtr; }
namespace osc { class ITabHost; }

namespace osc
{
    class RendererGeometryShaderTab final : public ITab {
    public:
        static CStringView id();

        explicit RendererGeometryShaderTab(const ParentPtr<ITabHost>&);
        RendererGeometryShaderTab(const RendererGeometryShaderTab&) = delete;
        RendererGeometryShaderTab(RendererGeometryShaderTab&&) noexcept;
        RendererGeometryShaderTab& operator=(const RendererGeometryShaderTab&) = delete;
        RendererGeometryShaderTab& operator=(RendererGeometryShaderTab&&) noexcept;
        ~RendererGeometryShaderTab() noexcept override;

    private:
        UID impl_get_id() const final;
        CStringView impl_get_name() const final;
        void impl_on_mount() final;
        void impl_on_unmount() final;
        bool impl_on_event(Event&) final;
        void impl_on_draw() final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
