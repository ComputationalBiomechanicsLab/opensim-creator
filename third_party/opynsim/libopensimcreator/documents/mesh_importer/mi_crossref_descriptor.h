#pragma once

#include <libopensimcreator/documents/mesh_importer/mi_crossref_direction.h>

#include <liboscar/utilities/c_string_view.h>
#include <liboscar/utilities/uid.h>

namespace osc
{
    class MiCrossrefDescriptor final {
    public:
        MiCrossrefDescriptor(UID connectee_id, CStringView label, MiCrossrefDirection direction) :
            connectee_id_{connectee_id},
            label_{label},
            direction_{direction}
        {}

        UID get_connectee_id() const { return connectee_id_; }

        CStringView get_label() const { return label_; }

        MiCrossrefDirection get_direction() const { return direction_; }

    private:
        UID connectee_id_;
        CStringView label_;
        MiCrossrefDirection direction_;
    };
}
