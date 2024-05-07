#include "FileMenu.h"

#include <oscar/UI/oscimgui.h>

void osc::mow::FileMenu::onDraw()
{
    if (ui::begin_menu("File")) {
        drawContent();
        ui::end_menu();
    }
}

void osc::mow::FileMenu::drawContent()
{
    ui::draw_text("TODO");
}
