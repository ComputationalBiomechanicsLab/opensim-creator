#pragma once

#include <libopensimcreator/Documents/MeshImporter/MiCrossrefDirection.h>

#include <liboscar/utilities/c_string_view.h>
#include <liboscar/utilities/uid.h>

namespace osc
{
    class MiCrossrefDescriptor final {
    public:
        MiCrossrefDescriptor(
            UID connecteeID_,
            CStringView label_,
            MiCrossrefDirection direction_) :

            m_ConnecteeID{connecteeID_},
            m_Label{label_},
            m_Direction{direction_}
        {
        }

        UID getConnecteeID() const
        {
            return m_ConnecteeID;
        }

        CStringView getLabel() const
        {
            return m_Label;
        }

        MiCrossrefDirection getDirection() const
        {
            return m_Direction;
        }
    private:
        UID m_ConnecteeID;
        CStringView m_Label;
        MiCrossrefDirection m_Direction;
    };
}
