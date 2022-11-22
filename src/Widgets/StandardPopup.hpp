#pragma once

#include "src/Widgets/Popup.hpp"

#include <glm/vec2.hpp>
#include <imgui.h>

#include <string>
#include <string_view>

namespace osc
{
    // base class for implementing a standard UI popup (that blocks the whole screen
    // apart from the popup content)
    class StandardPopup : public Popup {
    protected:
        StandardPopup(StandardPopup const&) = default;
        StandardPopup(StandardPopup&&) noexcept = default;
        StandardPopup& operator=(StandardPopup const&) = default;
        StandardPopup& operator=(StandardPopup&&) noexcept = default;
    public:
        virtual ~StandardPopup() noexcept = default;

        explicit StandardPopup(
            std::string_view popupName
        );

        StandardPopup(
            std::string_view popupName,
            float width,
            float height,
            ImGuiWindowFlags
        );

    protected:
        bool isPopupOpenedThisFrame() const;
        void requestClose();
        bool isModal() const;
        void setModal(bool);

    private:
        // this standard implementation supplies these
        bool implIsOpen() const final;
        void implOpen() final;
        void implClose() final;
        bool implBeginPopup() final;
        void implDrawPopupContent() final;
        void implEndPopup() final;

        // derivers can/must provide these
        virtual void implDrawContent() = 0;
        virtual void implOnClose()
        {
        }

        std::string m_PopupName;
        glm::ivec2 m_Dimensions;
        int m_PopupFlags;
        bool m_ShouldOpen;
        bool m_ShouldClose;
        bool m_JustOpened;
        bool m_IsOpen;
        bool m_IsModal;
    };
}
