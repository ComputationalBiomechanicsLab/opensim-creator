#include "Document.h"

#include <libopensimcreator/Documents/MeshImporter/Ground.h>
#include <libopensimcreator/Documents/MeshImporter/MIIDs.h>

#include <liboscar/Utils/ClonePtr.h>

osc::mi::Document::Document() :
    m_Objects{{MIIDs::Ground(), make_cloneable<Ground>()}}
{}
