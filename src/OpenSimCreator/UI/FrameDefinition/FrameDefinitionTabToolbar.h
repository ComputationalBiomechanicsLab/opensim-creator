#pragma once

#include <OpenSimCreator/Documents/Model/UndoableModelStatePair.h>

#include <oscar/Platform/Widget.h>
#include <oscar/Utils/LifetimedPtr.h>

#include <cstddef>
#include <memory>
#include <string_view>

namespace osc
{
    class FrameDefinitionTabToolbar final {
    public:
        FrameDefinitionTabToolbar(
            Widget& parent_,
            std::string_view label_,
            std::shared_ptr<UndoableModelStatePair>
        );

        void onDraw();
    private:
        void drawContent();
        void drawExportToOpenSimButton();
        void drawExportToOpenSimTooltipContent(size_t);

        LifetimedPtr<Widget> m_Parent;
        std::string m_Label;
        std::shared_ptr<UndoableModelStatePair> m_Model;
    };
}
