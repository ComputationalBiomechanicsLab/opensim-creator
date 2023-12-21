#pragma once

#include <OpenSimCreator/Documents/Frames/FrameDefinition.hpp>

#include <oscar/Utils/ClonePtr.hpp>

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

namespace OpenSim { class Model; }
namespace osc::mow { class ModelWarpConfiguration; }

namespace osc::mow
{
    class FrameDefinitionLookup final {
    public:
        FrameDefinitionLookup() = default;

        FrameDefinitionLookup(
            std::filesystem::path const& modelPath,
            OpenSim::Model const&,
            ModelWarpConfiguration const&
        );

        bool hasFrameDefinitionFile() const
        {
            return m_FrameDefinitionFileExists;
        }

        std::filesystem::path recommendedFrameDefinitionFilepath() const
        {
            return m_ExpectedFrameDefinitionFilepath;
        }

        frames::FrameDefinition const* lookup(std::string const& frameComponentAbsPath) const
        {
            if (auto const it = m_Lut.find(frameComponentAbsPath); it != m_Lut.end())
            {
                return &it->second;
            }
            else
            {
                return nullptr;
            }
        }
    private:
        std::filesystem::path m_ExpectedFrameDefinitionFilepath;
        bool m_FrameDefinitionFileExists;
        std::unordered_map<std::string, frames::FrameDefinition> m_Lut;
    };
}
