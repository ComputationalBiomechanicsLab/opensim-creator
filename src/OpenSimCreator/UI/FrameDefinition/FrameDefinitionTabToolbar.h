#pragma once

#include <OpenSimCreator/Documents/Model/UndoableModelStatePair.h>

#include <oscar/UI/Tabs/ITabHost.h>
#include <oscar/Utils/ParentPtr.h>

#include <cstddef>
#include <memory>
#include <string_view>

namespace osc
{
    class FrameDefinitionTabToolbar final {
    public:
        FrameDefinitionTabToolbar(
            std::string_view label_,
            ParentPtr<ITabHost>,
            std::shared_ptr<UndoableModelStatePair>
        );

        void onDraw();
    private:
        void drawContent();
        void drawExportToOpenSimButton();
        void drawExportToOpenSimTooltipContent(size_t);

        std::string m_Label;
        ParentPtr<ITabHost> m_TabHost;
        std::shared_ptr<UndoableModelStatePair> m_Model;
    };
}
