#pragma once

#include <OpenSim/Common/ComponentPath.h>
#include <oscar/Maths/CoordinateDirection.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Utils/ParentPtr.h>

#include <memory>
#include <optional>
#include <string>

namespace OpenSim { class Mesh; }
namespace OpenSim { class Point; }
namespace osc::fd { class CrossProductEdge; }
namespace osc::fd { class PointToPointEdge; }
namespace osc::fd { class Edge; }
namespace osc { class ITabHost; }
namespace osc { class UndoableModelStatePair; }

namespace osc::fd
{
    void ActionAddSphereInMeshFrame(
        UndoableModelStatePair&,
        const OpenSim::Mesh&,
        const std::optional<Vec3>& maybeClickPosInGround
    );

    void ActionAddOffsetFrameInMeshFrame(
        UndoableModelStatePair&,
        const OpenSim::Mesh&,
        const std::optional<Vec3>& maybeClickPosInGround
    );

    void ActionAddPointToPointEdge(
        UndoableModelStatePair&,
        const OpenSim::Point& ,
        const OpenSim::Point&
    );

    void ActionAddMidpoint(
        UndoableModelStatePair&,
        const OpenSim::Point&,
        const OpenSim::Point&
    );

    void ActionAddCrossProductEdge(
        UndoableModelStatePair&,
        const Edge&,
        const Edge&
    );

    void ActionSwapSocketAssignments(
        UndoableModelStatePair&,
        OpenSim::ComponentPath componentAbsPath,
        std::string firstSocketName,
        std::string secondSocketName
    );

    void ActionSwapPointToPointEdgeEnds(
        UndoableModelStatePair&,
        const PointToPointEdge&
    );

    void ActionSwapCrossProductEdgeOperands(
        UndoableModelStatePair&,
        const CrossProductEdge&
    );

    void ActionAddFrame(
        const std::shared_ptr<UndoableModelStatePair>&,
        const Edge& firstEdge,
        CoordinateDirection firstEdgeAxis,
        const Edge& otherEdge,
        const OpenSim::Point& origin
    );

    void ActionCreateBodyFromFrame(
        const std::shared_ptr<UndoableModelStatePair>&,
        const OpenSim::ComponentPath& frameAbsPath,
        const OpenSim::ComponentPath& meshAbsPath,
        const OpenSim::ComponentPath& jointFrameAbsPath,
        const OpenSim::ComponentPath& parentFrameAbsPath
    );
}
