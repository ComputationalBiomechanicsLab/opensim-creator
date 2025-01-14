#pragma once

#include <libOpenSimCreator/Documents/Model/UndoableModelStatePair.h>

#include <liboscar/Platform/Widget.h>
#include <liboscar/Utils/LifetimedPtr.h>

#include <cstddef>
#include <memory>
#include <string_view>

namespace osc
{
    class FrameDefinitionTabToolbar final {
    public:
        FrameDefinitionTabToolbar(
            std::string_view,
            Widget&,
            std::shared_ptr<UndoableModelStatePair>
        );

        void onDraw();
    private:
        void drawContent();
        void drawExportToOpenSimButton();
        void drawExportToOpenSimTooltipContent(size_t);

        std::string m_Label;
        LifetimedPtr<Widget> m_Parent;
        std::shared_ptr<UndoableModelStatePair> m_Model;
    };
}
