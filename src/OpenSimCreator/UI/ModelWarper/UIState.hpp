#pragma once

#include <OpenSimCreator/Documents/ModelWarper/Document.hpp>

#include <memory>

namespace osc::mow
{
    class UIState final {
    private:
        std::shared_ptr<Document> m_Document = std::make_shared<Document>();
    };
}
