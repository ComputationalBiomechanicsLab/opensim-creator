#pragma once

#include <liboscar/utilities/uid.h>

namespace osc
{
    // a collection of senteniel IDs used in the mesh importer
    class MiIDs final {
    public:
        static UID ground() { return get_all_ids().ground; }
        static UID empty() { return get_all_ids().empty; }
        static UID right_clicked_nothing() { return get_all_ids().right_clicked_nothing; }
        static UID ground_group() { return get_all_ids().ground_group; }
        static UID mesh_group() { return get_all_ids().mesh_group; }
        static UID body_group() { return get_all_ids().body_group; }
        static UID joint_group() { return get_all_ids().joint_group; }
        static UID station_group() { return get_all_ids().station_group; }
        static UID edge_group() { return get_all_ids().edge_group; }

    private:
        struct AllIDs final {
            UID ground;
            UID empty;
            UID right_clicked_nothing;
            UID ground_group;
            UID mesh_group;
            UID body_group;
            UID joint_group;
            UID station_group;
            UID edge_group;
        };
        static const AllIDs& get_all_ids();
    };
}
