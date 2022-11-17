#include "UndoRedoPanel.hpp"

#include "src/Utils/UndoRedo.hpp"
#include "src/Widgets/StandardPanel.hpp"

#include <imgui.h>

#include <cstddef>
#include <memory>
#include <string_view>
#include <utility>

class osc::UndoRedoPanel::Impl final : public osc::StandardPanel {
public:
    Impl(
        std::string_view panelName_,
        std::shared_ptr<osc::UndoRedo> storage_) :

        StandardPanel{std::move(panelName_)},
        m_Storage{std::move(storage_)}
    {
    }

private:
    void implDrawContent() override
    {
        if (ImGui::Button("undo"))
        {
            m_Storage->undo();
        }

        ImGui::SameLine();

        if (ImGui::Button("redo"))
        {
            m_Storage->redo();
        }

        int imguiID = 0;

        // draw undo entries oldest (highest index) to newest (lowest index)
        for (ptrdiff_t i = m_Storage->getNumUndoEntriesi()-1; 0 <= i && i < m_Storage->getNumUndoEntriesi(); --i)
        {
            ImGui::PushID(imguiID++);
            if (ImGui::Selectable(m_Storage->getUndoEntry(i).getMessage().c_str()))
            {
                m_Storage->undoTo(i);
            }
            ImGui::PopID();
        }

        ImGui::PushID(imguiID++);
        ImGui::Text("  %s", m_Storage->getHead().getMessage().c_str());
        ImGui::PopID();

        // draw redo entries oldest (lowest index) to newest (highest index)
        for (size_t i = 0; i < m_Storage->getNumRedoEntries(); ++i)
        {
            ImGui::PushID(imguiID++);
            if (ImGui::Selectable(m_Storage->getRedoEntry(i).getMessage().c_str()))
            {
                m_Storage->redoTo(i);
            }
            ImGui::PopID();
        }
    }

    std::shared_ptr<osc::UndoRedo> m_Storage;
};


// public API (PIMPL)

osc::UndoRedoPanel::UndoRedoPanel(std::string_view panelName_, std::shared_ptr<osc::UndoRedo> storage_) :
    m_Impl{std::make_unique<Impl>(std::move(panelName_), std::move(storage_))}
{
}

osc::UndoRedoPanel::~UndoRedoPanel() noexcept = default;

bool osc::UndoRedoPanel::implIsOpen() const
{
    return m_Impl->isOpen();
}

void osc::UndoRedoPanel::implOpen()
{
    return m_Impl->open();
}

void osc::UndoRedoPanel::implClose()
{
    return m_Impl->close();
}

void osc::UndoRedoPanel::implDraw()
{
    return m_Impl->draw();
}