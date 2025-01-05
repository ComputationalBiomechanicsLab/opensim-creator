#include "MeshImporterSharedState.h"

#include <oscar/Platform/Event.h>
#include <oscar/Platform/Events.h>

bool osc::mi::MeshImporterSharedState::onEvent(Event& ev)
{
    if (const auto* dropfile = dynamic_cast<const DropFileEvent*>(&ev)) {
        m_DroppedFiles.emplace_back(dropfile->path());
        return true;
    }
    return false;
}
