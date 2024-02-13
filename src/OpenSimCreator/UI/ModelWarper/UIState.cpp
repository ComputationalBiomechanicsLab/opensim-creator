#include "UIState.h"

#include <OpenSimCreator/Utils/OpenSimHelpers.h>

#include <oscar/Platform/os.h>

#include <filesystem>
#include <memory>
#include <optional>
#include <utility>

void osc::mow::UIState::actionOpenOsimOrPromptUser(std::optional<std::filesystem::path> path)
{
    if (!path) {
        path = PromptUserForFile("osim");
    }

    if (path) {
        m_Document = std::make_shared<Document>(std::move(path).value());
    }
}
