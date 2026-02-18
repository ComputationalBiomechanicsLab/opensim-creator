#include "MiDocument.h"

#include <libopensimcreator/Documents/MeshImporter/MiGround.h>
#include <libopensimcreator/Documents/MeshImporter/MiIDs.h>

#include <liboscar/utilities/clone_ptr.h>

osc::MiDocument::MiDocument() :
    m_Objects{{MiIDs::Ground(), make_cloneable<MiGround>()}}
{}
