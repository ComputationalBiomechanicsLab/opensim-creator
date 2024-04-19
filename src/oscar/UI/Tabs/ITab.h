#pragma once

#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/UID.h>

#include <SDL_events.h>

namespace osc { class ITabHost; }

namespace osc
{
    // a virtual interface to a single UI tab/workspace
    class ITab {
    protected:
        ITab() = default;
        ITab(const ITab&) = default;
        ITab(ITab&&) noexcept = default;
        ITab& operator=(const ITab&) = default;
        ITab& operator=(ITab&&) noexcept = default;
    public:
        virtual ~ITab() noexcept = default;

        UID getID() const { return implGetID(); }
        CStringView getName() const { return implGetName(); }
        bool isUnsaved() const { return implIsUnsaved(); }
        bool trySave() { return implTrySave(); }
        void onMount() { implOnMount(); }
        void onUnmount() { implOnUnmount(); }
        bool onEvent(const SDL_Event& e) { return implOnEvent(e); }
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
        virtual bool implOnEvent(const SDL_Event&) { return false; }
        virtual void implOnTick() {}
        virtual void implOnDrawMainMenu() {}
        virtual void implOnDraw() = 0;
    };
}
