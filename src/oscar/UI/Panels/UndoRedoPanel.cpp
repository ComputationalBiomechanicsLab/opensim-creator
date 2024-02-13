#include "UndoRedoPanel.h"

#include <oscar/UI/Panels/StandardPanelImpl.h>
#include <oscar/Utils/UndoRedo.h>

#include <imgui.h>

#include <cstddef>
#include <memory>
#include <string_view>
#include <utility>

using namespace osc;

class osc::UndoRedoPanel::Impl final : public StandardPanelImpl {
public:
    Impl(
        std::string_view panelName,
        std::shared_ptr<UndoRedoBase> storage_) :

        StandardPanelImpl{panelName},
        m_Storage{std::move(storage_)}
    {
    }

private:
    void implDrawContent() final
    {
        UndoRedoPanel::DrawContent(*m_Storage);
    }

    std::shared_ptr<UndoRedoBase> m_Storage;
};


// public API (PIMPL)

void osc::UndoRedoPanel::DrawContent(UndoRedoBase& storage)
{
    if (ImGui::Button("undo"))
    {
        storage.undo();
    }

    ImGui::SameLine();

    if (ImGui::Button("redo"))
    {
        storage.redo();
    }

    int imguiID = 0;

    // draw undo entries oldest (highest index) to newest (lowest index)
    for (ptrdiff_t i = storage.getNumUndoEntriesi()-1; 0 <= i && i < storage.getNumUndoEntriesi(); --i)
    {
        ImGui::PushID(imguiID++);
        if (ImGui::Selectable(storage.getUndoEntry(i).message().c_str()))
        {
            storage.undoTo(i);
        }
        ImGui::PopID();
    }

    ImGui::PushID(imguiID++);
    ImGui::Text("  %s", storage.getHead().message().c_str());
    ImGui::PopID();

    // draw redo entries oldest (lowest index) to newest (highest index)
    for (ptrdiff_t i = 0; i < storage.getNumRedoEntriesi(); ++i)
    {
        ImGui::PushID(imguiID++);
        if (ImGui::Selectable(storage.getRedoEntry(i).message().c_str()))
        {
            storage.redoTo(i);
        }
        ImGui::PopID();
    }
}

osc::UndoRedoPanel::UndoRedoPanel(std::string_view panelName_, std::shared_ptr<UndoRedoBase> storage_) :
    m_Impl{std::make_unique<Impl>(panelName_, std::move(storage_))}
{
}

osc::UndoRedoPanel::UndoRedoPanel(UndoRedoPanel&&) noexcept = default;
osc::UndoRedoPanel& osc::UndoRedoPanel::operator=(UndoRedoPanel&&) noexcept = default;
osc::UndoRedoPanel::~UndoRedoPanel() noexcept = default;

CStringView osc::UndoRedoPanel::implGetName() const
{
    return m_Impl->getName();
}

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

void osc::UndoRedoPanel::implOnDraw()
{
    return m_Impl->onDraw();
}
