#pragma once

#include <OpenSimCreator/Documents/ModelWarper/FrameWarpLookup.h>
#include <OpenSimCreator/Documents/ModelWarper/MeshWarpLookup.h>
#include <OpenSimCreator/Documents/ModelWarper/ModelWarpConfiguration.h>
#include <OpenSimCreator/Documents/ModelWarper/ValidationCheck.h>
#include <OpenSimCreator/Documents/ModelWarper/ValidationState.h>
#include <OpenSimCreator/Documents/ModelWarper/WarpDetail.h>

#include <oscar/Utils/ClonePtr.h>

#include <filesystem>
#include <vector>

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

        OpenSim::Model const& model() const { return *m_Model; }

        std::vector<WarpDetail> details(OpenSim::Mesh const&) const;
        std::vector<ValidationCheck> validate(OpenSim::Mesh const&) const;
        ValidationState state(OpenSim::Mesh const&) const;

        std::vector<WarpDetail> details(OpenSim::PhysicalOffsetFrame const&) const;
        std::vector<ValidationCheck> validate(OpenSim::PhysicalOffsetFrame const&) const;
        ValidationState state(OpenSim::PhysicalOffsetFrame const&) const;

    private:
        ClonePtr<OpenSim::Model const> m_Model;
        ModelWarpConfiguration m_ModelWarpConfig;
        MeshWarpLookup m_MeshWarpLookup;
        FrameWarpLookup m_FrameWarpLookup;
    };
}
