#include "SaveChangesPopup.hpp"

#include "src/Widgets/StandardPopup.hpp"

#include <imgui.h>

#include <utility>

class osc::SaveChangesPopup::Impl final : public StandardPopup {
public:

    Impl(SaveChangesPopupConfig config) :
        StandardPopup{config.popupName},
        m_Config{std::move(config)}
    {
    }

    void implDraw() override
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

osc::SaveChangesPopup::SaveChangesPopup(SaveChangesPopupConfig config) :
    m_Impl{new Impl{std::move(config)}}
{
}

osc::SaveChangesPopup::SaveChangesPopup(SaveChangesPopup&& tmp) noexcept :
    m_Impl{std::exchange(tmp.m_Impl, nullptr)}
{
}

osc::SaveChangesPopup& osc::SaveChangesPopup::operator=(SaveChangesPopup&& tmp) noexcept
{
    std::swap(m_Impl, tmp.m_Impl);
    return *this;
}

osc::SaveChangesPopup::~SaveChangesPopup() noexcept
{
    delete m_Impl;
}

bool osc::SaveChangesPopup::isOpen() const
{
    return m_Impl->isOpen();
}

void osc::SaveChangesPopup::open()
{
    m_Impl->open();
}

void osc::SaveChangesPopup::close()
{
    m_Impl->close();
}

void osc::SaveChangesPopup::draw()
{
    m_Impl->draw();
}
