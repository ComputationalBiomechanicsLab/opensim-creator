#include "FrameDefinitionLookup.hpp"

namespace
{
    std::filesystem::path CalcExpectedFrameDefinitionFileLocation(
        std::filesystem::path const& modelFilePath)
    {
        std::filesystem::path expected = modelFilePath;
        expected.replace_extension(".frames.toml");
        expected = std::filesystem::weakly_canonical(expected);
        return expected;
    }
}

osc::mow::FrameDefinitionLookup::FrameDefinitionLookup(
    std::filesystem::path const& modelPath,
    OpenSim::Model const&,
    ModelWarpConfiguration const&) :

    m_ExpectedFrameDefinitionFilepath{CalcExpectedFrameDefinitionFileLocation(modelPath)},
    m_FrameDefinitionFileExists{std::filesystem::exists(m_ExpectedFrameDefinitionFilepath)}
{
    // TODO: go find frames, or least-squares, etc.
}
