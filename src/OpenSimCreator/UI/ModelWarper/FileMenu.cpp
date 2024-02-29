#include "FileMenu.h"

#include <oscar/UI/oscimgui.h>

void osc::mow::FileMenu::onDraw()
{
    if (ui::BeginMenu("File")) {
        drawContent();
        ui::EndMenu();
    }
}

void osc::mow::FileMenu::drawContent()
{
    ui::Text("TODO");
}
