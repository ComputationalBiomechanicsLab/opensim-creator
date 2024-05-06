#pragma once

#include <oscar/UI/Widgets/IPopup.h>

#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace osc { class IMainUIStateAPI; }
namespace osc { template<typename T> class ParentPtr; }
namespace osc { class IModelStatePair; }

namespace osc
{
    class ModelStatePairContextMenu final : public IPopup {
    public:
        ModelStatePairContextMenu(
            std::string_view panelName,
            std::shared_ptr<IModelStatePair>,
            ParentPtr<IMainUIStateAPI> const&,
            std::optional<std::string> maybeComponentAbsPath
        );
        ModelStatePairContextMenu(ModelStatePairContextMenu const&) = delete;
        ModelStatePairContextMenu(ModelStatePairContextMenu&&) noexcept;
        ModelStatePairContextMenu& operator=(ModelStatePairContextMenu const&) = delete;
        ModelStatePairContextMenu& operator=(ModelStatePairContextMenu&&) noexcept;
        ~ModelStatePairContextMenu() noexcept;

    private:
        bool impl_is_open() const final;
        void impl_open() final;
        void impl_close() final;
        bool impl_begin_popup() final;
        void impl_on_draw() final;
        void impl_end_popup() final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
