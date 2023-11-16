#pragma once

#include <OpenSimCreator/ModelGraph/CrossrefDirection.hpp>

#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/UID.hpp>

namespace osc
{
    class CrossrefDescriptor final {
    public:
        CrossrefDescriptor(
            UID connecteeID_,
            CStringView label_,
            CrossrefDirection direction_) :

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

        CrossrefDirection getDirection() const
        {
            return m_Direction;
        }
    private:
        UID m_ConnecteeID;
        CStringView m_Label;
        CrossrefDirection m_Direction;
    };
}