#include "FrameDefinitionLookup.hpp"

#include <OpenSimCreator/Documents/Frames/FrameDefinition.hpp>
#include <OpenSimCreator/Documents/Frames/FramesFile.hpp>
#include <OpenSimCreator/Documents/Frames/FramesHelpers.hpp>

#include <oscar/Utils/StdVariantHelpers.hpp>

#include <exception>
#include <filesystem>
#include <fstream>
#include <string>
#include <variant>

using osc::frames::FrameDefinition;
using osc::mow::FrameDefinitionLookup;
using osc::frames::FramesFile;
using osc::frames::ReadFramesFromTOML;

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
    OpenSim::Model const&) :

    m_ExpectedFrameDefinitionFilepath{CalcExpectedFrameDefinitionFileLocation(modelPath)},
    m_FramesFileOrLoadError{TryLoadFramesFile(m_ExpectedFrameDefinitionFilepath)}
{
}

bool osc::mow::FrameDefinitionLookup::hasFrameDefinitionFile() const
{
    return std::holds_alternative<FramesFile>(m_FramesFileOrLoadError);
}

std::filesystem::path osc::mow::FrameDefinitionLookup::recommendedFrameDefinitionFilepath() const
{
    return m_ExpectedFrameDefinitionFilepath;
}

bool osc::mow::FrameDefinitionLookup::hasFramesFileLoadError() const
{
    return std::holds_alternative<std::string>(m_FramesFileOrLoadError);
}

std::optional<std::string> osc::mow::FrameDefinitionLookup::getFramesFileLoadError() const
{
    return std::visit(Overload
    {
        [](std::string const& error) { return std::optional<std::string>{error}; },
        [](auto const&) { return std::optional<std::string>{}; }
    }, m_FramesFileOrLoadError);
}

FrameDefinition const* osc::mow::FrameDefinitionLookup::lookup(std::string const& frameComponentAbsPath) const
{
    return std::visit(Overload
    {
        [&frameComponentAbsPath](frames::FramesFile const& f) { return f.findFrameDefinitionByName(frameComponentAbsPath); },
        [](auto const&) { return static_cast<FrameDefinition const*>(nullptr); }
    }, m_FramesFileOrLoadError);
}

FrameDefinitionLookup::InnerVariant osc::mow::FrameDefinitionLookup::TryLoadFramesFile(std::filesystem::path const& framesFile)
{
    if (!std::filesystem::exists(framesFile))
    {
        return FileDoesntExist{};
    }

    std::ifstream f{framesFile};
    if (!f)
    {
        return std::string{"could not open frames file for reading"};
    }

    try
    {
        return ReadFramesFromTOML(f);
    }
    catch (std::exception const& ex)
    {
        return std::string{ex.what()};
    }
}
