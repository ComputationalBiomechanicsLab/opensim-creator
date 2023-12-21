#pragma once

#include <OpenSimCreator/Documents/ModelWarper/Detail.hpp>
#include <OpenSimCreator/Documents/ModelWarper/FrameDefinitionLookup.hpp>
#include <OpenSimCreator/Documents/ModelWarper/MeshWarpPairing.hpp>
#include <OpenSimCreator/Documents/ModelWarper/MeshWarpPairingLookup.hpp>
#include <OpenSimCreator/Documents/ModelWarper/ModelWarpConfiguration.hpp>
#include <OpenSimCreator/Documents/ModelWarper/ValidationCheck.hpp>
#include <OpenSimCreator/Documents/ModelWarper/ValidationCheckConsumerResponse.hpp>

#include <oscar/Utils/ClonePtr.hpp>

#include <filesystem>

namespace OpenSim { class Mesh; }
namespace OpenSim { class Model; }
namespace OpenSim { class PhysicalOffsetFrame; }

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

        size_t getNumWarpableMeshesInModel() const;
        void forEachWarpableMeshInModel(std::function<void(OpenSim::Mesh const&)> const&) const;
        void forEachMeshWarpDetail(OpenSim::Mesh const&, std::function<void(Detail)> const&) const;
        void forEachMeshWarpCheck(OpenSim::Mesh const&, std::function<ValidationCheckConsumerResponse(ValidationCheck)> const&) const;
        ValidationCheck::State getMeshWarpState(OpenSim::Mesh const&) const;

        size_t getNumWarpableFramesInModel() const;
        void forEachWarpableFrameInModel(std::function<void(OpenSim::PhysicalOffsetFrame const&)> const&) const;
    private:
        ClonePtr<OpenSim::Model const> m_Model;
        ModelWarpConfiguration m_TopLevelWarpConfig;
        MeshWarpPairingLookup m_MeshWarpPairingLookup;
        FrameDefinitionLookup m_FrameDefinitionLookup;
    };
}
