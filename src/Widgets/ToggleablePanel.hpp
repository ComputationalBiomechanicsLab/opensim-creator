#pragma once

#include "src/Widgets/ToggleablePanelFlags.hpp"
#include "src/Utils/CStringView.hpp"

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace osc { class Panel; }

namespace osc
{
    // a panel that the user may be able to toggle at runtime
    class ToggleablePanel final {
    public:
        ToggleablePanel(
            std::string_view name_,
            std::function<std::shared_ptr<osc::Panel>(std::string_view)> constructorFunc_,
            ToggleablePanelFlags flags_ = ToggleablePanelFlags_Default);
        ToggleablePanel(ToggleablePanel const&) = delete;
        ToggleablePanel(ToggleablePanel&&) noexcept;
        ToggleablePanel& operator=(ToggleablePanel const&) = delete;
        ToggleablePanel& operator=(ToggleablePanel&&) noexcept;
        ~ToggleablePanel() noexcept;

        osc::CStringView getName() const;

        bool isEnabledByDefault() const;
        bool isActivated() const;
        void activate();
        void deactivate();
        void toggleActivation();

        void draw();

        // clear any instance data if the panel has been closed
        void garbageCollect();

    private:
        std::string m_Name;
        std::function<std::shared_ptr<osc::Panel>(std::string_view)> m_ConstructorFunc;
        ToggleablePanelFlags m_Flags;
        std::optional<std::shared_ptr<osc::Panel>> m_Instance;
    };
}