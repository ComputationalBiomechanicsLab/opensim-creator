#pragma once

#include <liboscar/Platform/Widget.h>

#include <cstddef>
#include <memory>
#include <string_view>

namespace osc { class UndoableModelStatePair; }

namespace osc
{
    class FrameDefinitionTabToolbar final : public Widget {
    public:
        explicit FrameDefinitionTabToolbar(
            Widget* parent,
            std::string_view name,
            std::shared_ptr<UndoableModelStatePair>
        );

    private:
        void impl_on_draw() final;

        void drawContent();
        void drawExportToOpenSimButton();
        void drawExportToOpenSimTooltipContent(size_t);

        std::shared_ptr<UndoableModelStatePair> m_Model;
    };
}
