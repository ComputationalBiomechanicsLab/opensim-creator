#pragma once

#include <functional>
#include <string>

namespace osc
{
    class SaveChangesPopupConfig {
    public:
        std::string popupName = "Save changes?";
        std::function<bool()> onUserClickedSave = []{ return true; };
        std::function<bool()> onUserClickedDontSave = []{ return true; };
        std::function<bool()> onUserCancelled = []{ return true; };
        std::string content = popupName;
    };

    class SaveChangesPopup {
    public:
        SaveChangesPopup(SaveChangesPopupConfig);
        SaveChangesPopup(SaveChangesPopup const&) = delete;
        SaveChangesPopup(SaveChangesPopup&&) noexcept;
        SaveChangesPopup& operator=(SaveChangesPopup const&) = delete;
        SaveChangesPopup& operator=(SaveChangesPopup&&) noexcept;
        ~SaveChangesPopup() noexcept;

        bool isOpen() const;
        void open();
        void close();
        void draw();

    private:
        class Impl;
        Impl* m_Impl;
    };
}
