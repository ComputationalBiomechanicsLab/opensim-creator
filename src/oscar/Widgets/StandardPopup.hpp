#pragma once

#include "oscar/Widgets/Popup.hpp"

#include <glm/vec2.hpp>
#include <imgui.h>

#include <optional>
#include <string>
#include <string_view>

namespace osc { struct Rect; }

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
            glm::vec2 dimensions,
            ImGuiWindowFlags
        );

    protected:
        bool isPopupOpenedThisFrame() const;
        void requestClose();
        bool isModal() const;
        void setModal(bool);
        void setRect(Rect const&);
        void setDimensions(glm::vec2);
        void setPosition(std::optional<glm::vec2>);

    private:
        // this standard implementation supplies these
        bool implIsOpen() const final;
        void implOpen() final;
        void implClose() final;
        bool implBeginPopup() final;
        void implOnDraw() final;
        void implEndPopup() final;

        // derivers can/must provide these
        virtual void implBeforeImguiBeginPopup() {}
        virtual void implAfterImguiBeginPopup() {}
        virtual void implDrawContent() = 0;
        virtual void implOnClose() {}

        std::string m_PopupName;
        glm::ivec2 m_Dimensions;
        std::optional<glm::ivec2> m_MaybePosition;
        ImGuiWindowFlags m_PopupFlags;
        bool m_ShouldOpen;
        bool m_ShouldClose;
        bool m_JustOpened;
        bool m_IsOpen;
        bool m_IsModal;
    };
}
