#pragma once

#include <memory>

namespace osc { class IModelStatePair; }
namespace osc { class Widget; }

namespace osc
{
    class ModelStatusBar final {
    public:
        ModelStatusBar(Widget&, std::shared_ptr<IModelStatePair>);
        ModelStatusBar(const ModelStatusBar&) = delete;
        ModelStatusBar(ModelStatusBar&&) noexcept;
        ModelStatusBar& operator=(const ModelStatusBar&) = delete;
        ModelStatusBar& operator=(ModelStatusBar&&) noexcept;
        ~ModelStatusBar() noexcept;

        void onDraw();

    private:
        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
