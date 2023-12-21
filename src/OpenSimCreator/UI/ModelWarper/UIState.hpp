#pragma once

#include <OpenSimCreator/Documents/ModelWarper/Document.hpp>
#include <OpenSimCreator/Documents/ModelWarper/MeshWarpPairing.hpp>

#include <filesystem>
#include <memory>
#include <optional>
#include <vector>

namespace OpenSim { class Mesh; }
namespace OpenSim { class Model; }
namespace OpenSim { class PhysicalOffsetFrame; }

namespace osc::mow
{
    class UIState final {
    public:
        OpenSim::Model const& getModel() const;
        std::vector<OpenSim::Mesh const*> getWarpableMeshes() const;
        std::vector<OpenSim::PhysicalOffsetFrame const*> getWarpableFrames() const;
        MeshWarpPairing const* findMeshWarp(OpenSim::Mesh const&) const;

        void actionOpenModel(std::optional<std::filesystem::path> path = std::nullopt);
    private:
        std::shared_ptr<Document> m_Document = std::make_shared<Document>();
    };
}
