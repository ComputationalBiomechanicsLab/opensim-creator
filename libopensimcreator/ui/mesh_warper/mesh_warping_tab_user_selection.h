#pragma once

#include <libopensimcreator/documents/mesh_warper/tps_document_element_id.h>

#include <unordered_set>
#include <utility>

namespace osc
{
    // the user's current selection
    struct MeshWaringTabUserSelection final {

        size_t size() const { return m_SelectedSceneElements.size(); }
        auto begin() const { return m_SelectedSceneElements.begin(); }
        auto end() const { return m_SelectedSceneElements.end(); }

        void clear() { m_SelectedSceneElements.clear(); }
        void select(TPSDocumentElementID el) { m_SelectedSceneElements.insert(std::move(el)); }
        bool contains(const TPSDocumentElementID& el) const { return m_SelectedSceneElements.contains(el); }
    private:
        std::unordered_set<TPSDocumentElementID> m_SelectedSceneElements;
    };
}
