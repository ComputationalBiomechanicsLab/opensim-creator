#pragma once

#include "src/Utils/CStringView.hpp"

#include <SDL_events.h>

#include <memory>

namespace osc
{
    class TabHost;

    class Tab {

    protected:
        explicit Tab(TabHost* parent);

    public:
        virtual ~Tab() noexcept = default;

        void onMount();
        void onUnmount();
        bool onEvent(SDL_Event const& e);
        void tick();
        void drawMainMenu();
        void draw();
        osc::CStringView name();

    protected:
        void addTab(std::unique_ptr<Tab>);
        void selectTab(Tab*);
        void closeTab(Tab*);
        TabHost* updParent() { return m_Parent; }

    private:
        virtual void implOnMount() {}
        virtual void implOnUnmount() {}
        virtual bool implOnEvent(SDL_Event const& e) { return false; }
        virtual void implOnTick() {}
        virtual void implDrawMainMenu() {}
        virtual void implOnDraw() = 0;
        virtual osc::CStringView implName() = 0;

        TabHost* m_Parent;
    };
}
