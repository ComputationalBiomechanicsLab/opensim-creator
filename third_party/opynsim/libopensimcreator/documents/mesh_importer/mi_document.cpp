#include "mi_document.h"

#include <libopensimcreator/documents/mesh_importer/mi_ground.h>
#include <libopensimcreator/documents/mesh_importer/mi_ids.h>

#include <liboscar/utilities/clone_ptr.h>

osc::MiDocument::MiDocument() :
    objects_{{MiIDs::ground(), make_cloneable<MiGround>()}}
{}
