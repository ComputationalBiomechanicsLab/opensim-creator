#pragma once

#include <OpenSimCreator/Documents/ModelWarper/Detail.h>
#include <OpenSimCreator/Documents/ModelWarper/Document.h>
#include <OpenSimCreator/Documents/ModelWarper/MeshWarpPairing.h>
#include <OpenSimCreator/Documents/ModelWarper/ValidationCheck.h>
#include <OpenSimCreator/Documents/ModelWarper/ValidationCheckConsumerResponse.h>

#include <filesystem>
#include <memory>
#include <optional>

namespace OpenSim { class Mesh; }
namespace OpenSim { class Model; }
namespace OpenSim { class PhysicalOffsetFrame; }

namespace osc::mow
{
    class UIState final {
    public:
        OpenSim::Model const& getModel() const;

        size_t getNumWarpableMeshesInModel() const;
        void forEachWarpableMeshInModel(
            std::function<void(OpenSim::Mesh const&)> const&
        ) const;
        void forEachMeshWarpDetail(
            OpenSim::Mesh const&,
            std::function<void(Detail)> const&
        ) const;
        void forEachMeshWarpCheck(
            OpenSim::Mesh const&,
            std::function<ValidationCheckConsumerResponse(ValidationCheck)> const&
        ) const;
        ValidationCheck::State getMeshWarpState(
            OpenSim::Mesh const&
        ) const;

        size_t getNumWarpableFramesInModel() const;
        void forEachWarpableFrameInModel(std::function<void(OpenSim::PhysicalOffsetFrame const&)> const&) const;
        void forEachFrameDefinitionCheck(
            OpenSim::PhysicalOffsetFrame const&,
            std::function<ValidationCheckConsumerResponse(ValidationCheck)> const&
        ) const;

        void actionOpenModel(std::optional<std::filesystem::path> path = std::nullopt);
    private:
        std::shared_ptr<Document> m_Document = std::make_shared<Document>();
    };
}
