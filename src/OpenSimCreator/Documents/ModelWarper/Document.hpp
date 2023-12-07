#pragma once

#include <OpenSimCreator/Documents/ModelWarper/FrameDefinitionLookup.hpp>
#include <OpenSimCreator/Documents/ModelWarper/MeshWarpPairingLookup.hpp>
#include <OpenSimCreator/Documents/ModelWarper/ModelWarpConfiguration.hpp>

#include <oscar/Utils/ClonePtr.hpp>

#include <filesystem>

namespace OpenSim { class Mesh; }
namespace OpenSim { class Model; }
namespace osc::mow { class MeshWarpPairing; }

namespace osc::mow
{
    // the document that the model warping UI represents/manipulates
    class Document final {
    public:
        Document();
        explicit Document(std::filesystem::path const& osimPath);
        Document(Document const&);
        Document(Document&&) noexcept;
        Document& operator=(Document const&);
        Document& operator=(Document&&) noexcept;
        ~Document() noexcept;

        OpenSim::Model const& getModel() const
        {
            return *m_Model;
        }

        MeshWarpPairing const* findMeshWarp(std::string const& meshComponentAbsPath) const
        {
            return m_MeshWarpPairingLookup.lookup(meshComponentAbsPath);
        }
    private:
        ClonePtr<OpenSim::Model const> m_Model;
        ModelWarpConfiguration m_TopLevelWarpConfig;
        MeshWarpPairingLookup m_MeshWarpPairingLookup;
        FrameDefinitionLookup m_FrameDefinitionLookup;
    };
}
