#include "Document.h"

#include <libOpenSimCreator/Documents/MeshImporter/Ground.h>
#include <libOpenSimCreator/Documents/MeshImporter/MIIDs.h>

#include <liboscar/Utils/ClonePtr.h>

osc::mi::Document::Document() :
    m_Objects{{MIIDs::Ground(), make_cloneable<Ground>()}}
{}
