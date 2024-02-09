#include "Document.h"

#include <OpenSimCreator/Documents/MeshImporter/Ground.h>
#include <OpenSimCreator/Documents/MeshImporter/MIIDs.h>

#include <oscar/Utils/ClonePtr.h>

osc::mi::Document::Document() :
    m_Objects{{MIIDs::Ground(), ClonePtr<MIObject>{Ground{}}}}
{
}
