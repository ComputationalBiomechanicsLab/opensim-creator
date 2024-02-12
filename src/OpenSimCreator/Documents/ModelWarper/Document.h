#pragma once

#include <OpenSimCreator/Documents/ModelWarper/Detail.h>
#include <OpenSimCreator/Documents/ModelWarper/FrameWarpLookup.h>
#include <OpenSimCreator/Documents/ModelWarper/MeshWarpLookup.h>
#include <OpenSimCreator/Documents/ModelWarper/ModelWarpConfiguration.h>
#include <OpenSimCreator/Documents/ModelWarper/ValidationCheck.h>
#include <OpenSimCreator/Documents/ModelWarper/ValidationCheckConsumerResponse.h>

#include <oscar/Utils/ClonePtr.h>

#include <filesystem>
#include <functional>

namespace OpenSim { class Mesh; }
namespace OpenSim { class Model; }
namespace OpenSim { class PhysicalOffsetFrame; }

namespace osc::mow
{
    // the document that the model warping UI represents/manipulates
    class Document final {
    public:
        Document();
        explicit Document(std::filesystem::path const& osimFileLocation);
        Document(Document const&);
        Document(Document&&) noexcept;
        Document& operator=(Document const&);
        Document& operator=(Document&&) noexcept;
        ~Document() noexcept;

        OpenSim::Model const& getModel() const
        {
            return *m_Model;
        }

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
        ValidationCheck::State getMeshWarpState(OpenSim::Mesh const&) const;

        size_t getNumWarpableFramesInModel() const;
        void forEachWarpableFrameInModel(
            std::function<void(OpenSim::PhysicalOffsetFrame const&)> const&
        ) const;
        void forEachFrameDefinitionCheck(
            OpenSim::PhysicalOffsetFrame const&,
            std::function<ValidationCheckConsumerResponse(ValidationCheck)> const&
        ) const;

    private:
        ClonePtr<OpenSim::Model const> m_Model;
        ModelWarpConfiguration m_ModelWarpConfig;
        MeshWarpLookup m_MeshWarpLookup;
        FrameWarpLookup m_FrameWarpLookup;
    };
}
