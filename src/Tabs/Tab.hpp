#pragma once

#include "src/Utils/CStringView.hpp"
#include "src/Utils/UID.hpp"

#include <SDL_events.h>

namespace osc
{
    class TabHost;

    class Tab {
    public:
        Tab() = default;
        Tab(Tab const&) = delete;
        Tab(Tab&&) noexcept = delete;
        Tab& operator=(Tab const&) = delete;
        Tab& operator=(Tab&&) noexcept = delete;
        virtual ~Tab() noexcept = default;

        UID getID() const;
        CStringView getName() const;
        TabHost* parent() const;
        bool isUnsaved() const;
        bool trySave();
        void onMount();
        void onUnmount();
        bool onEvent(SDL_Event const& e);
        void onTick();
        void onDrawMainMenu();
        void onDraw();

    private:
        virtual UID implGetID() const = 0;
        virtual CStringView implGetName() const = 0;
        virtual TabHost* implParent() const = 0;
        virtual bool implIsUnsaved() const { return false; }
        virtual bool implTrySave() { return true; }
        virtual void implOnMount() {}
        virtual void implOnUnmount() {}
        virtual bool implOnEvent(SDL_Event const&) { return false; }
        virtual void implOnTick() {}
        virtual void implOnDrawMainMenu() {}
        virtual void implOnDraw() = 0;
    };
}
