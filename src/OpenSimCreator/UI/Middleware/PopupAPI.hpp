#pragma once

#include <oscar/UI/Widgets/Popup.hpp>

#include <memory>

namespace osc
{
	class PopupAPI {
	protected:
		PopupAPI() = default;
		PopupAPI(PopupAPI const&) = default;
		PopupAPI(PopupAPI&&) noexcept = default;
		PopupAPI& operator=(PopupAPI const&) = default;
		PopupAPI& operator=(PopupAPI&&) noexcept = default;
	public:
		virtual ~PopupAPI() noexcept = default;

		void pushPopup(std::unique_ptr<Popup> p)
		{
			implPushPopup(std::move(p));
		}
	private:
		virtual void implPushPopup(std::unique_ptr<Popup>) = 0;
	};
}