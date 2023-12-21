#pragma once

#include <OpenSimCreator/Documents/ModelWarper/FrameDefinitionLookup.hpp>
#include <OpenSimCreator/Documents/ModelWarper/MeshWarpPairingLookup.hpp>
#include <OpenSimCreator/Documents/ModelWarper/ModelWarpConfiguration.hpp>

#include <oscar/Utils/ClonePtr.hpp>

#include <filesystem>

namespace OpenSim { class Model; }
namespace osc::frames { class FrameDefinition; }
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

        bool hasFrameDefinitionFile() const
        {
            return m_FrameDefinitionLookup.hasFrameDefinitionFile();
        }

        std::filesystem::path recommendedFrameDefinitionFilepath() const
        {
            return m_FrameDefinitionLookup.recommendedFrameDefinitionFilepath();
        }

        bool hasFramesFileLoadError() const
        {
            return m_FrameDefinitionLookup.hasFramesFileLoadError();
        }

        std::optional<std::string> getFramesFileLoadError() const
        {
            return m_FrameDefinitionLookup.getFramesFileLoadError();
        }

        OpenSim::Model const& getModel() const
        {
            return *m_Model;
        }

        MeshWarpPairing const* findMeshWarp(std::string const& meshComponentAbsPath) const
        {
            return m_MeshWarpPairingLookup.lookup(meshComponentAbsPath);
        }

        frames::FrameDefinition const* findFrameDefinition(std::string const& frameComponentAbsPath) const
        {
            return m_FrameDefinitionLookup.lookup(frameComponentAbsPath);
        }
    private:
        ClonePtr<OpenSim::Model const> m_Model;
        ModelWarpConfiguration m_TopLevelWarpConfig;
        MeshWarpPairingLookup m_MeshWarpPairingLookup;
        FrameDefinitionLookup m_FrameDefinitionLookup;
    };
}
