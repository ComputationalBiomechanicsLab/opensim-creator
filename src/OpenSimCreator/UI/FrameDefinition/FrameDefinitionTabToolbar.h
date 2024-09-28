#pragma once

#include <OpenSimCreator/Documents/Model/UndoableModelStatePair.h>
#include <OpenSimCreator/UI/IMainUIStateAPI.h>

#include <oscar/Platform/Widget.h>
#include <oscar/Utils/ParentPtr.h>

#include <cstddef>
#include <memory>
#include <string_view>

namespace osc
{
    class FrameDefinitionTabToolbar final {
    public:
        FrameDefinitionTabToolbar(
            std::string_view,
            ParentPtr<IMainUIStateAPI>,
            std::shared_ptr<UndoableModelStatePair>
        );

        void onDraw();
    private:
        void drawContent();
        void drawExportToOpenSimButton();
        void drawExportToOpenSimTooltipContent(size_t);

        std::string m_Label;
        ParentPtr<IMainUIStateAPI> m_TabHost;
        std::shared_ptr<UndoableModelStatePair> m_Model;
    };
}
