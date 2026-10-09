#include "mi_document.h"

#include <libopensimcreator/documents/mesh_importer/mi_ground.h>
#include <libopensimcreator/documents/mesh_importer/mi_ids.h>

#include <liboscar/utilities/clone_ptr.h>

osc::MiDocument::MiDocument() :
    m_Objects{{MiIDs::Ground(), make_cloneable<MiGround>()}}
{}
