#pragma once

#include <memory>
#include <string>
#include <string_view>

namespace osc
{
	class UndoableModelStatePair;
}

namespace osc
{
	class ModelMusclePlotPanel final {
	public:
		ModelMusclePlotPanel(std::shared_ptr<UndoableModelStatePair>, std::string_view panelName);
		ModelMusclePlotPanel(ModelMusclePlotPanel const&) = delete;
		ModelMusclePlotPanel(ModelMusclePlotPanel&&) noexcept;
		ModelMusclePlotPanel& operator=(ModelMusclePlotPanel const&) = delete;
		ModelMusclePlotPanel& operator=(ModelMusclePlotPanel&&) noexcept;
		~ModelMusclePlotPanel() noexcept;

		std::string const& getName() const;
		bool isOpen() const;
		void open();
		void close();
		void draw();

		class Impl;
	private:
		Impl* m_Impl;
	};
}