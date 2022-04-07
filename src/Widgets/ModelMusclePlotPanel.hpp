#pragma once

#include <memory>
#include <string_view>

namespace osc
{
	class UndoableUiModel;
}

namespace osc
{
	class ModelMusclePlotPanel final {
	public:
		ModelMusclePlotPanel(std::shared_ptr<UndoableUiModel>, std::string_view panelName);
		ModelMusclePlotPanel(ModelMusclePlotPanel const&) = delete;
		ModelMusclePlotPanel(ModelMusclePlotPanel&&) noexcept;
		ModelMusclePlotPanel& operator=(ModelMusclePlotPanel const&) = delete;
		ModelMusclePlotPanel& operator=(ModelMusclePlotPanel&&) noexcept;
		~ModelMusclePlotPanel() noexcept;

		void open();
		void close();
		void draw();

		class Impl;
	private:
		std::unique_ptr<Impl> m_Impl;
	};
}