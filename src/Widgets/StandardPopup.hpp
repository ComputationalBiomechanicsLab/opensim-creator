#pragma once

#include <string>
#include <string_view>

namespace osc
{
    // base class for implementing a standard UI popup (that blocks the whole screen
    // apart from the popup content)
    class StandardPopup {
    public:
        explicit StandardPopup(std::string_view popupName);

        StandardPopup(
            std::string_view popupName,
            float width,
            float height,
            int popupFlags  // ImGuiWindowFlags
        );
        virtual ~StandardPopup() noexcept = default;

        bool isOpen() const;
        void open();
        void close();
        void draw();

    protected:
        bool isPopupOpenedThisFrame() const;
        void requestClose();

    private:
        virtual void implDraw() = 0;
        virtual void implOnClose() {}

        std::string m_PopupName;
        float m_Width;
        float m_Height;
        int m_PopupFlags;
        bool m_ShouldOpen;
        bool m_ShouldClose;
        bool m_JustOpened;
        bool m_IsOpen;
    };
}
