#pragma once

#include <OpenSimCreator/Documents/MeshWarp/TPSDocumentElementID.hpp>

#include <utility>
#include <unordered_set>

namespace osc
{
    // the user's current selection
    struct MeshWaringTabUserSelection final {

        void clear()
        {
            m_SelectedSceneElements.clear();
        }

        void select(TPSDocumentElementID el)
        {
            m_SelectedSceneElements.insert(std::move(el));
        }

        bool contains(TPSDocumentElementID const& el) const
        {
            return m_SelectedSceneElements.find(el) != m_SelectedSceneElements.end();
        }

        std::unordered_set<TPSDocumentElementID> const& getUnderlyingSet() const
        {
            return m_SelectedSceneElements;
        }

    private:
        std::unordered_set<TPSDocumentElementID> m_SelectedSceneElements;
    };
}
