#include "Document.hpp"

#include <OpenSim/Simulation/Model/Model.h>

osc::mow::Document::Document() :
    m_Model{std::make_unique<OpenSim::Model>()}
{
}

osc::mow::Document::Document(std::filesystem::path osimPath) :
    m_Model{std::make_unique<OpenSim::Model>(osimPath.string())}
{
}

osc::mow::Document::Document(Document const&) = default;
osc::mow::Document::Document(Document&&) noexcept = default;
osc::mow::Document& osc::mow::Document::operator=(Document const&) = default;
osc::mow::Document& osc::mow::Document::operator=(Document&&) noexcept = default;
osc::mow::Document::~Document() noexcept = default;
