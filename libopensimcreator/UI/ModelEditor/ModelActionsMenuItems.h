#pragma once

#include <memory>

namespace osc { class IModelStatePair; }
namespace osc { class Widget; }

namespace osc
{
    class ModelActionsMenuItems final {
    public:
        ModelActionsMenuItems(Widget&, std::shared_ptr<IModelStatePair>);
        ModelActionsMenuItems(const ModelActionsMenuItems&) = delete;
        ModelActionsMenuItems(ModelActionsMenuItems&&) noexcept;
        ModelActionsMenuItems& operator=(const ModelActionsMenuItems&) = delete;
        ModelActionsMenuItems& operator=(ModelActionsMenuItems&&) noexcept;
        ~ModelActionsMenuItems() noexcept;

        void onDraw();

    private:
        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
