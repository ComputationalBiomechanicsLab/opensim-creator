#pragma once

#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/UID.hpp>

#include <SDL_events.h>

namespace osc { class ITabHost; }

namespace osc
{
    // a virtual interface to a single UI tab/workspace
    class ITab {
    protected:
        ITab() = default;
        ITab(ITab const&) = default;
        ITab(ITab&&) noexcept = default;
        ITab& operator=(ITab const&) = default;
        ITab& operator=(ITab&&) noexcept = default;
    public:
        virtual ~ITab() noexcept = default;

        UID getID() const { return implGetID(); }
        CStringView getName() const { return implGetName(); }
        bool isUnsaved() const { return implIsUnsaved(); }
        bool trySave() { return implTrySave(); }
        void onMount() { implOnMount(); }
        void onUnmount() { implOnUnmount(); }
        bool onEvent(SDL_Event const& e) { return implOnEvent(e); }
        void onTick() { implOnTick(); }
        void onDrawMainMenu() { implOnDrawMainMenu(); }
        void onDraw() { implOnDraw(); }

    private:
        virtual UID implGetID() const = 0;
        virtual CStringView implGetName() const = 0;
        virtual bool implIsUnsaved() const { return false; }
        virtual bool implTrySave() { return true; }
        virtual void implOnMount() {}
        virtual void implOnUnmount() {}
        virtual bool implOnEvent(SDL_Event const&) { return false;}
        virtual void implOnTick() {}
        virtual void implOnDrawMainMenu() {}
        virtual void implOnDraw() = 0;
    };
}
