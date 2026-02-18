#include "mesh_importer_shared_state.h"

#include <liboscar/platform/events/drop_file_event.h>

bool osc::MeshImporterSharedState::onEvent(Event& ev)
{
    if (const auto* dropfile = dynamic_cast<const DropFileEvent*>(&ev)) {
        m_DroppedFiles.emplace_back(dropfile->path());
        return true;
    }
    return false;
}
