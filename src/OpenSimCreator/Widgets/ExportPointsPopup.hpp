#pragma once

#include <oscar/Widgets/Popup.hpp>

#include <memory>
#include <string_view>

namespace osc { class VirtualConstModelStatePair; }

namespace osc
{
	class ExportPointsPopup final : public Popup {
	public:
		ExportPointsPopup(
			std::string_view popupName,
			std::shared_ptr<VirtualConstModelStatePair const>
		);
		ExportPointsPopup(ExportPointsPopup const&) = delete;
		ExportPointsPopup(ExportPointsPopup&&) noexcept;
		ExportPointsPopup& operator=(ExportPointsPopup const&) = delete;
		ExportPointsPopup& operator=(ExportPointsPopup&&) noexcept;
		~ExportPointsPopup() noexcept;

	private:
		bool implIsOpen() const final;
		void implOpen() final;
		void implClose() final;
		bool implBeginPopup() final;
		void implDrawPopupContent() final;
		void implEndPopup() final;

		class Impl;
		std::unique_ptr<Impl> m_Impl;
	};
}