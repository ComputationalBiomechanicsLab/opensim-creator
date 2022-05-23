#pragma once

#include "src/Tabs/Tab.hpp"
#include "src/Utils/CStringView.hpp"
#include "src/Utils/UID.hpp"

#include <SDL_events.h>

#include <memory>

namespace osc
{
	class MainUIStateAPI;
	class UndoableModelStatePair;
}

namespace osc
{
	class ModelEditorTab final : public Tab {
	public:
		ModelEditorTab(MainUIStateAPI*, std::unique_ptr<UndoableModelStatePair>);
		ModelEditorTab(ModelEditorTab const&) = delete;
		ModelEditorTab(ModelEditorTab&&) noexcept;
		ModelEditorTab& operator=(ModelEditorTab const&) = delete;
		ModelEditorTab& operator=(ModelEditorTab&&) noexcept;
		~ModelEditorTab() noexcept override;

	private:
		UID implGetID() const override;
		CStringView implGetName() const override;
		TabHost* implParent() const override;
		bool implIsUnsaved() const override;
		void implOnMount() override;
		void implOnUnmount() override;
		bool implOnEvent(SDL_Event const&) override;
		void implOnTick() override;
		void implOnDrawMainMenu() override;
		void implOnDraw() override;

		class Impl;
		Impl* m_Impl;
	};
}