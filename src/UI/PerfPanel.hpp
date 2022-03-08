#pragma once

#include <memory>
#include <string_view>

namespace osc
{
    class PerfPanel final {
    public:
        PerfPanel(std::string_view panelName);
        PerfPanel(PerfPanel const&) = delete;
        PerfPanel(PerfPanel&&) noexcept;
        PerfPanel& operator=(PerfPanel const&) = delete;
        PerfPanel& operator=(PerfPanel&&) noexcept;
        ~PerfPanel();

        void open();
        void close();
        bool draw();

        class Impl;
    private:
        std::unique_ptr<Impl> m_Impl;
    };
}
