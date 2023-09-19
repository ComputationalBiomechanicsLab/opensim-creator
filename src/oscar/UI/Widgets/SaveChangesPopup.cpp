#include "SaveChangesPopup.hpp"

#include <oscar/UI/Widgets/SaveChangesPopupConfig.hpp>
#include <oscar/UI/Widgets/StandardPopup.hpp>

#include <imgui.h>

#include <utility>

class osc::SaveChangesPopup::Impl final : public StandardPopup {
public:

    explicit Impl(SaveChangesPopupConfig config) :
        StandardPopup{config.popupName},
        m_Config{std::move(config)}
    {
    }

    void implDrawContent() final
    {
        ImGui::TextUnformatted(m_Config.content.c_str());

        if (ImGui::Button("Yes"))
        {
            if (m_Config.onUserClickedSave())
            {
                close();
            }
        }

        ImGui::SameLine();

        if (ImGui::Button("No"))
        {
            if (m_Config.onUserClickedDontSave())
            {
                close();
            }
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel"))
        {
            if (m_Config.onUserCancelled())
            {
                close();
            }
        }
    }
private:
    SaveChangesPopupConfig m_Config;
};


// public API (PIMPL)

osc::SaveChangesPopup::SaveChangesPopup(SaveChangesPopupConfig const& config) :
    m_Impl{std::make_unique<Impl>(config)}
{
}

osc::SaveChangesPopup::SaveChangesPopup(SaveChangesPopup&&) noexcept = default;
osc::SaveChangesPopup& osc::SaveChangesPopup::operator=(SaveChangesPopup&&) noexcept = default;
osc::SaveChangesPopup::~SaveChangesPopup() noexcept = default;

bool osc::SaveChangesPopup::implIsOpen() const
{
    return m_Impl->isOpen();
}

void osc::SaveChangesPopup::implOpen()
{
    m_Impl->open();
}

void osc::SaveChangesPopup::implClose()
{
    m_Impl->close();
}

bool osc::SaveChangesPopup::implBeginPopup()
{
    return m_Impl->beginPopup();
}

void osc::SaveChangesPopup::implOnDraw()
{
    m_Impl->onDraw();
}

void osc::SaveChangesPopup::implEndPopup()
{
    m_Impl->endPopup();
}

void osc::SaveChangesPopup::onDraw()
{
    if (m_Impl->beginPopup())
    {
        m_Impl->onDraw();
        m_Impl->endPopup();
    }
}
