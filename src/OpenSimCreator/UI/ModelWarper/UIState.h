#pragma once

#include <OpenSimCreator/Documents/ModelWarper/Document.h>
#include <OpenSimCreator/Documents/ModelWarper/ValidationCheck.h>
#include <OpenSimCreator/Documents/ModelWarper/ValidationCheckConsumerResponse.h>
#include <OpenSimCreator/Documents/ModelWarper/WarpDetail.h>

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
        OpenSim::Model const& model() const;

        void forEachWarpDetail(
            OpenSim::Mesh const&,
            std::function<void(WarpDetail)> const&
        ) const;
        void forEachWarpCheck(
            OpenSim::Mesh const&,
            std::function<ValidationCheckConsumerResponse(ValidationCheck)> const&
        ) const;
        ValidationCheck::State warpState(
            OpenSim::Mesh const&
        ) const;

        void forEachWarpCheck(
            OpenSim::PhysicalOffsetFrame const&,
            std::function<ValidationCheckConsumerResponse(ValidationCheck)> const&
        ) const;

        void actionOpenOsimOrPromptUser(
            std::optional<std::filesystem::path> maybeOsimPath = std::nullopt
        );
    private:
        std::shared_ptr<Document> m_Document = std::make_shared<Document>();
    };
}
