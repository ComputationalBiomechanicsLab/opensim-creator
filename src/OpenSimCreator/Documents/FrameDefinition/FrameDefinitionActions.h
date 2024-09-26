#pragma once

#include <OpenSim/Common/ComponentPath.h>
#include <oscar/Maths/CoordinateDirection.h>
#include <oscar/Maths/Vec3.h>

#include <memory>
#include <optional>
#include <string>

namespace OpenSim { class Mesh; }
namespace OpenSim { class Point; }
namespace osc::fd { class CrossProductEdge; }
namespace osc::fd { class PointToPointEdge; }
namespace osc::fd { class Edge; }
namespace osc { class IModelStatePair; }
namespace osc { class ITabHost; }

namespace osc::fd
{
    void ActionAddSphereInMeshFrame(
        IModelStatePair&,
        const OpenSim::Mesh&,
        const std::optional<Vec3>& maybeClickPosInGround
    );

    void ActionAddOffsetFrameInMeshFrame(
        IModelStatePair&,
        const OpenSim::Mesh&,
        const std::optional<Vec3>& maybeClickPosInGround
    );

    void ActionAddPointToPointEdge(
        IModelStatePair&,
        const OpenSim::Point& ,
        const OpenSim::Point&
    );

    void ActionAddMidpoint(
        IModelStatePair&,
        const OpenSim::Point&,
        const OpenSim::Point&
    );

    void ActionAddCrossProductEdge(
        IModelStatePair&,
        const Edge&,
        const Edge&
    );

    void ActionSwapSocketAssignments(
        IModelStatePair&,
        OpenSim::ComponentPath componentAbsPath,
        std::string firstSocketName,
        std::string secondSocketName
    );

    void ActionSwapPointToPointEdgeEnds(
        IModelStatePair&,
        const PointToPointEdge&
    );

    void ActionSwapCrossProductEdgeOperands(
        IModelStatePair&,
        const CrossProductEdge&
    );

    void ActionAddFrame(
        const std::shared_ptr<IModelStatePair>&,
        const Edge& firstEdge,
        CoordinateDirection firstEdgeAxis,
        const Edge& otherEdge,
        const OpenSim::Point& origin
    );

    void ActionCreateBodyFromFrame(
        const std::shared_ptr<IModelStatePair>&,
        const OpenSim::ComponentPath& frameAbsPath,
        const OpenSim::ComponentPath& meshAbsPath,
        const OpenSim::ComponentPath& jointFrameAbsPath,
        const OpenSim::ComponentPath& parentFrameAbsPath
    );
}
