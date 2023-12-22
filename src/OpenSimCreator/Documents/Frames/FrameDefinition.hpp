#pragma once

#include <OpenSimCreator/Documents/Frames/FrameAxis.hpp>

#include <string>
#include <utility>

namespace osc::frames
{
    class FrameDefinition final {
    public:
        FrameDefinition(
            std::string name_,
            std::string associatedMeshName_,
            std::string originLocation_,
            std::string axisEdgeBegin_,
            std::string axisEdgeEnd_,
            FrameAxis axisEdgeAxis_,
            std::string nonParallelEdgeBegin_,
            std::string nonParallelEdgeEnd_,
            FrameAxis crossProductEdgeAxis_) :

            m_Name{std::move(name_)},
            m_AssociatedMeshName{std::move(associatedMeshName_)},
            m_OriginLocationLandmarkName{std::move(originLocation_)},
            m_AxisEdgeBeginLandmarkName{std::move(axisEdgeBegin_)},
            m_AxisEdgeEndLandmarkName{std::move(axisEdgeEnd_)},
            m_AxisEdgeAxis{axisEdgeAxis_},
            m_NonParallelEdgeBeginLandmarkName{std::move(nonParallelEdgeBegin_)},
            m_NonParallelEdgeEndLandmarkName{std::move(nonParallelEdgeEnd_)},
            m_CrossProductEdgeAxis{crossProductEdgeAxis_}
        {
        }

        std::string const& getName() const
        {
            return m_Name;
        }
        std::string const& getAssociatedMeshName() const
        {
            return m_AssociatedMeshName;
        }
        std::string const& getOriginLocationLandmarkName() const
        {
            return m_OriginLocationLandmarkName;
        }
        std::string const& getAxisEdgeBeginLandmarkName() const
        {
            return m_AxisEdgeBeginLandmarkName;
        }
        std::string const& getAxisEdgeEndLandmarkName() const
        {
            return m_AxisEdgeEndLandmarkName;
        }
        FrameAxis getAxisEdgeAxis() const
        {
            return m_AxisEdgeAxis;
        }
        std::string const& getNonParallelEdgeBeginLandmarkName() const
        {
            return m_NonParallelEdgeBeginLandmarkName;
        }
        std::string const& getNonParallelEdgeEndLandmarkName() const
        {
            return m_NonParallelEdgeEndLandmarkName;
        }
        FrameAxis getCrossProductEdgeAxis() const
        {
            return m_CrossProductEdgeAxis;
        }

        friend bool operator==(FrameDefinition const&, FrameDefinition const&) = default;
    private:
        std::string m_Name;
        std::string m_AssociatedMeshName;
        std::string m_OriginLocationLandmarkName;
        std::string m_AxisEdgeBeginLandmarkName;
        std::string m_AxisEdgeEndLandmarkName;
        FrameAxis m_AxisEdgeAxis;
        std::string m_NonParallelEdgeBeginLandmarkName;
        std::string m_NonParallelEdgeEndLandmarkName;
        FrameAxis m_CrossProductEdgeAxis;
    };
}
