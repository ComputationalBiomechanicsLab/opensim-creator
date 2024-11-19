#pragma once

#include <concepts>
#include <memory>
#include <utility>
#include <vector>

namespace osc { class IPopup; }

namespace osc
{
    // generic storage for a drawable popup stack
    class PopupManager final {
    public:
        PopupManager();
        PopupManager(const PopupManager&) = delete;
        PopupManager(PopupManager&&) noexcept;
        PopupManager& operator=(const PopupManager&) = delete;
        PopupManager& operator=(PopupManager&&) noexcept;
        ~PopupManager() noexcept;

        void push_back(std::shared_ptr<IPopup> ptr)
        {
            popups_.push_back(std::move(ptr));
        }

        template<std::derived_from<IPopup> T, typename... Args>
        requires std::constructible_from<T, Args&&...>
        T& emplace_back(Args&&... args)
        {
            auto ptr = std::make_shared<T>(std::forward<Args>(args)...);
            T& ref = *ptr;
            popups_.push_back(std::move(ptr));
            return ref;
        }

        void on_mount() { open_all(); }
        void open_all();
        void on_draw();
        [[nodiscard]] bool empty();
        void clear();
    private:
        std::vector<std::shared_ptr<IPopup>> popups_;
    };
}
