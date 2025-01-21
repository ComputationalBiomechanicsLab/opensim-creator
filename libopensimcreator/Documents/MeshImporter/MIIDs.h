#pragma once

#include <liboscar/Utils/UID.h>

namespace osc::mi
{
    // a collection of senteniel IDs used in the mesh importer
    class MIIDs final {
    public:
        static UID Ground() { return GetAllIDs().Ground; }
        static UID Empty() { return GetAllIDs().Empty; }
        static UID RightClickedNothing() { return GetAllIDs().RightClickedNothing; }
        static UID GroundGroup() { return GetAllIDs().GroundGroup; }
        static UID MeshGroup() { return GetAllIDs().MeshGroup; }
        static UID BodyGroup() { return GetAllIDs().BodyGroup; }
        static UID JointGroup() { return GetAllIDs().JointGroup; }
        static UID StationGroup() { return GetAllIDs().StationGroup; }
        static UID EdgeGroup() { return GetAllIDs().EdgeGroup; }
    private:
        struct AllIDs final {
            UID Ground;
            UID Empty;
            UID RightClickedNothing;
            UID GroundGroup;
            UID MeshGroup;
            UID BodyGroup;
            UID JointGroup;
            UID StationGroup;
            UID EdgeGroup;
        };
        static const AllIDs& GetAllIDs();
    };
}
