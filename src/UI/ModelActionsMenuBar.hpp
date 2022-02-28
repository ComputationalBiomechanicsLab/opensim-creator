#pragma once

#include <memory>

namespace osc
{
    class UiModel;
}

namespace osc
{
    class ModelActionsMenuBar final {
    public:
        ModelActionsMenuBar();
        ModelActionsMenuBar(ModelActionsMenuBar const&) = delete;
        ModelActionsMenuBar(ModelActionsMenuBar&&) noexcept;
        ModelActionsMenuBar& operator=(ModelActionsMenuBar const&) = delete;
        ModelActionsMenuBar& operator=(ModelActionsMenuBar&&) noexcept;
        ~ModelActionsMenuBar() noexcept;

        bool draw(UiModel&);

        class Impl;
    private:
        std::unique_ptr<Impl> m_Impl;
    };
}
