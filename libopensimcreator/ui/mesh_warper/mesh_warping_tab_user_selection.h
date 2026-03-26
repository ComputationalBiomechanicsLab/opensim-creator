#pragma once

#include <libopensimcreator/documents/mesh_warper/mw_document_element_id.h>

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
        void select(MwDocumentElementID el) { m_SelectedSceneElements.insert(std::move(el)); }
        bool contains(const MwDocumentElementID& el) const { return m_SelectedSceneElements.contains(el); }
    private:
        std::unordered_set<MwDocumentElementID> m_SelectedSceneElements;
    };
}
