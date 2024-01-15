#pragma once

#include <OpenSimCreator/Documents/FrameDefinition/MaybeNegatedAxis.hpp>

#include <OpenSim/Common/ComponentPath.h>
#include <oscar/Maths/Vec3.hpp>
#include <oscar/Utils/ParentPtr.hpp>

#include <memory>
#include <optional>
#include <string>

namespace OpenSim { class Mesh; }
namespace OpenSim { class Point; }
namespace osc::fd { class FDCrossProductEdge; }
namespace osc::fd { class FDPointToPointEdge; }
namespace osc::fd { class FDVirtualEdge; }
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
        FDVirtualEdge const&,
        FDVirtualEdge const&
    );

    void ActionSwapSocketAssignments(
        UndoableModelStatePair&,
        OpenSim::ComponentPath componentAbsPath,
        std::string firstSocketName,
        std::string secondSocketName
    );

    void ActionSwapPointToPointEdgeEnds(
        UndoableModelStatePair&,
        FDPointToPointEdge const&
    );

    void ActionSwapCrossProductEdgeOperands(
        UndoableModelStatePair&,
        FDCrossProductEdge const&
    );

    void ActionAddFrame(
        std::shared_ptr<UndoableModelStatePair> const&,
        FDVirtualEdge const& firstEdge,
        MaybeNegatedAxis firstEdgeAxis,
        FDVirtualEdge const& otherEdge,
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
