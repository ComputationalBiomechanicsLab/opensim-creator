#pragma once

#include <oscar/Utils/ClonePtr.hpp>

#include <filesystem>

namespace OpenSim { class Model; }

namespace osc::mow
{
    class Document final {
    public:
        Document();
        explicit Document(std::filesystem::path osimPath);
        Document(Document const&);
        Document(Document&&) noexcept;
        Document& operator=(Document const&);
        Document& operator=(Document&&) noexcept;
        ~Document() noexcept;
    private:
        ClonePtr<OpenSim::Model> m_Model;
    };
}
