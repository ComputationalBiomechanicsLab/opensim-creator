#pragma once

#include <string>
#include <string_view>

namespace osc
{
	// base class for implementing a panel that has a name
	class NamedPanel {
	public:
		explicit NamedPanel(std::string_view name);
		NamedPanel(std::string_view name, int imGuiWindowFlags);

		virtual ~NamedPanel() noexcept = default;

		bool isOpen() const;
		void open();
		void close();
		void draw();

	protected:
		void requestClose();

	private:
		virtual void implDraw() = 0;

		std::string m_PanelName;
		int m_PanelFlags;
	};
}