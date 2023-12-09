#pragma once

#include <string>

namespace osc::frames
{
    class FrameDefinition final {
    private:
        std::string m_FrameName;
        std::string m_AssociatedMeshName;
        std::string m_OriginLandmarkName;
        std::string m_AxisEdgeBeginName;
        std::string m_AxisEdgeEndName;
        // TODO: axis_edge_axis
        std::string m_NonParallelEdgeBeginLandmarkName;
        std::string m_NonParallelEdgeEndLandmarkName;
        // TODO: cross_product_edge_axis
    };
}
