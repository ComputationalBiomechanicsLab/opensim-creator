#pragma once

#include <OpenSimCreator/Documents/FrameDefinition/MaybeNegatedAxis.h>

#include <OpenSim/Common/ComponentPath.h>
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
        OpenSim::Mesh const&,
        std::optional<Vec3> const& maybeClickPosInGround
    );

    void ActionAddOffsetFrameInMeshFrame(
        UndoableModelStatePair&,
        OpenSim::Mesh const&,
        std::optional<Vec3> const& maybeClickPosInGround
    );

    void ActionAddPointToPointEdge(
        UndoableModelStatePair&,
        OpenSim::Point const& ,
        OpenSim::Point const&
    );

    void ActionAddMidpoint(
        UndoableModelStatePair&,
        OpenSim::Point const&,
        OpenSim::Point const&
    );

    void ActionAddCrossProductEdge(
        UndoableModelStatePair&,
        Edge const&,
        Edge const&
    );

    void ActionSwapSocketAssignments(
        UndoableModelStatePair&,
        OpenSim::ComponentPath componentAbsPath,
        std::string firstSocketName,
        std::string secondSocketName
    );

    void ActionSwapPointToPointEdgeEnds(
        UndoableModelStatePair&,
        PointToPointEdge const&
    );

    void ActionSwapCrossProductEdgeOperands(
        UndoableModelStatePair&,
        CrossProductEdge const&
    );

    void ActionAddFrame(
        std::shared_ptr<UndoableModelStatePair> const&,
        Edge const& firstEdge,
        MaybeNegatedAxis firstEdgeAxis,
        Edge const& otherEdge,
        OpenSim::Point const& origin
    );

    void ActionCreateBodyFromFrame(
        std::shared_ptr<UndoableModelStatePair> const&,
        OpenSim::ComponentPath const& frameAbsPath,
        OpenSim::ComponentPath const& meshAbsPath,
        OpenSim::ComponentPath const& jointFrameAbsPath,
        OpenSim::ComponentPath const& parentFrameAbsPath
    );
}
