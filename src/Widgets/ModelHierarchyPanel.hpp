#pragma once

#include <string_view>

namespace OpenSim
{
    class Component;
}

namespace osc
{
    class VirtualConstModelStatePair;
}

namespace osc
{
    class ModelHierarchyPanel final {
    public:
        enum class ResponseType {
            NothingHappened,
            SelectionChanged,
            HoverChanged,
        };

        struct Response final {
            OpenSim::Component const* ptr = nullptr;
            ResponseType type = ResponseType::NothingHappened;
        };

        ModelHierarchyPanel(std::string_view panelName);
        ModelHierarchyPanel(ModelHierarchyPanel const&) = delete;
        ModelHierarchyPanel(ModelHierarchyPanel&&) noexcept;
        ModelHierarchyPanel& operator=(ModelHierarchyPanel const&) = delete;
        ModelHierarchyPanel& operator=(ModelHierarchyPanel&&) noexcept;
        ~ModelHierarchyPanel() noexcept;

        bool isOpen() const;
        void open();
        void close();
        Response draw(VirtualConstModelStatePair const&);

    private:
        class Impl;
        Impl* m_Impl;
    };
}
