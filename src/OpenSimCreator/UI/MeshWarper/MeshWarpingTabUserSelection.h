#pragma once

#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentElementID.h>

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

        bool contains(const TPSDocumentElementID& el) const
        {
            return m_SelectedSceneElements.contains(el);
        }

        std::unordered_set<TPSDocumentElementID> const& getUnderlyingSet() const
        {
            return m_SelectedSceneElements;
        }

    private:
        std::unordered_set<TPSDocumentElementID> m_SelectedSceneElements;
    };
}
