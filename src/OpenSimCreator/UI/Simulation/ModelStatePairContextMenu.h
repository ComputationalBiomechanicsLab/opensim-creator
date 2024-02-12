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
        bool implIsOpen() const final;
        void implOpen() final;
        void implClose() final;
        bool implBeginPopup() final;
        void implOnDraw() final;
        void implEndPopup() final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
