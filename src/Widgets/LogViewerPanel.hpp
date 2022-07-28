#pragma once

#include <string_view>

namespace osc
{
    class LogViewerPanel final {
    public:
        LogViewerPanel(std::string_view panelName);
        LogViewerPanel(LogViewerPanel const&) = delete;
        LogViewerPanel(LogViewerPanel&&) noexcept;
        LogViewerPanel& operator=(LogViewerPanel const&) = delete;
        LogViewerPanel& operator=(LogViewerPanel&&) noexcept;
        ~LogViewerPanel() noexcept;

        bool isOpen() const;
        void open();
        void close();
        void draw();

    private:
        class Impl;
        Impl* m_Impl;
    };
}
