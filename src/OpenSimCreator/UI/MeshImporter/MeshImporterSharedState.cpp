#include "MeshImporterSharedState.h"

#include <oscar/Platform/Event.h>
#include <SDL_events.h>

bool osc::mi::MeshImporterSharedState::onEvent(const Event& ev)
{
    const SDL_Event& e = ev;

    // if the user drags + drops a file into the window, assume it's a meshfile
    // and start loading it
    if (e.type == SDL_DROPFILE && e.drop.file != nullptr)
    {
        m_DroppedFiles.emplace_back(e.drop.file);
        return true;
    }

    return false;
}
