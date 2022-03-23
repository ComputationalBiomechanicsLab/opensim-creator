#pragma once

#include <memory>

namespace osc
{
    class UndoableUiModel;
}

namespace osc
{
    class ModelActionsMenuBar final {
    public:
        ModelActionsMenuBar(std::shared_ptr<UndoableUiModel>);
        ModelActionsMenuBar(ModelActionsMenuBar const&) = delete;
        ModelActionsMenuBar(ModelActionsMenuBar&&) noexcept;
        ModelActionsMenuBar& operator=(ModelActionsMenuBar const&) = delete;
        ModelActionsMenuBar& operator=(ModelActionsMenuBar&&) noexcept;
        ~ModelActionsMenuBar() noexcept;

        bool draw();

        class Impl;
    private:
        std::unique_ptr<Impl> m_Impl;
    };
}
