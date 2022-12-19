#include "FrameDefinitionTab.hpp"

#include "src/Utils/Cow.hpp"
#include "src/Utils/CStringView.hpp"
#include "src/Utils/UID.hpp"

#include <glm/vec3.hpp>
#include <IconsFontAwesome5.h>
#include <SDL_events.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace
{
    // immutable "class" (i.e. non-instance) data associated with an object
    class FDClass final {
    public:
        FDClass(
            osc::CStringView name_,
            osc::CStringView description_) :

            m_Name{std::move(name_)},
            m_Description{std::move(description_)}
        {
        }

        osc::UID getID() const
        {
            return m_ID;
        }

        osc::CStringView getName() const
        {
            return m_Name;
        }

        osc::CStringView getDescription() const
        {
            return m_Description;
        }

    private:
        osc::UID m_ID;
        std::string m_Name;
        std::string m_Description;
    };

    // forward decls for all supported scene elements
    class FDBasicLandmarkEl;

    // a visitor for each possible concrete scene element type (cannot modify the scene element)
    class FDConstVisitor {
    protected:
        FDConstVisitor() = default;
        FDConstVisitor(FDConstVisitor const&) = default;
        FDConstVisitor(FDConstVisitor&&) noexcept = default;
        FDConstVisitor& operator=(FDConstVisitor const&) = default;
        FDConstVisitor& operator=(FDConstVisitor&&) noexcept = default;
    public:
        virtual ~FDConstVisitor() noexcept = default;

        void operator()(FDBasicLandmarkEl const& el)
        {
            implVisit(el);
        }

    private:
        virtual void implVisit(FDBasicLandmarkEl const&) = 0;
    };

    // a visitor for each possible concrete scene element type (can modify the scene element)
    class FDVisitor {
    protected:
        FDVisitor() = default;
        FDVisitor(FDVisitor const&) = default;
        FDVisitor(FDVisitor&&) noexcept = default;
        FDVisitor& operator=(FDVisitor const&) = default;
        FDVisitor& operator=(FDVisitor&&) noexcept = default;
    public:
        virtual ~FDVisitor() noexcept = default;

        void operator()(FDBasicLandmarkEl const& el)
        {
            implVisit(el);
        }

    private:
        virtual void implVisit(FDBasicLandmarkEl const& el) = 0;
    };

    // abstract base class that all scene elements implement
    class VirtualFDEl {
    protected:
        VirtualFDEl() = default;
        VirtualFDEl(VirtualFDEl const&) = default;
        VirtualFDEl(VirtualFDEl&&) noexcept = default;
        VirtualFDEl& operator=(VirtualFDEl const&) = default;
        VirtualFDEl& operator=(VirtualFDEl&&) noexcept = default;
    public:
        virtual ~VirtualFDEl() noexcept = default;

        FDClass const& getClass() const
        {
            return implGetClass();
        }

        osc::UID getID() const
        {
            return implGetID();
        }

        osc::CStringView getLabel() const
        {
            return implGetLabel();
        }

        void setLabel(osc::CStringView newLabel)
        {
            implSetLabel(std::move(newLabel));
        }

        void accept(FDConstVisitor& visitor) const
        {
            implAccept(visitor);
        }

        void accept(FDVisitor& visitor)
        {
            implAccept(visitor);
        }

    private:
        virtual FDClass const& implGetClass() const = 0;
        virtual osc::UID implGetID() const = 0;
        virtual osc::CStringView implGetLabel() const = 0;
        virtual void implSetLabel(osc::CStringView) = 0;
        virtual void implAccept(FDConstVisitor&) const = 0;
        virtual void implAccept(FDVisitor&) = 0;
    };

    // a scene element that represents a basic landmark (location in space)
    class FDBasicLandmarkEl final : public VirtualFDEl {
    public:
        static FDClass const& getClass()
        {
            static FDClass const s_Class =
            {
                "BasicLandmark",
                "A user-defined location in 3D space",
            };
            return s_Class;
        }

        glm::vec3 const& getLocation() const
        {
            return m_Location;
        }

        void setLocation(glm::vec3 const& newLocation)
        {
            m_Location = newLocation;
        }

    private:
        FDClass const& implGetClass() const final
        {
            return FDBasicLandmarkEl::getClass();
        }

        osc::UID implGetID() const final
        {
            return m_ID;
        }

        osc::CStringView implGetLabel() const final
        {
            return m_Label;
        }

        void implSetLabel(osc::CStringView newLabel) final
        {
            m_Label = std::move(newLabel);
        }

        void implAccept(FDConstVisitor& visitor) const final
        {
            visitor(*this);
        }

        void implAccept(FDVisitor& visitor) final
        {
            visitor(*this);
        }

        osc::UID m_ID;
        std::string m_Label;
        glm::vec3 m_Location;
    };

    // an instance of a "document" that the user is editing
    class FDDocument final {
    public:

    private:
        std::unordered_map<osc::UID, osc::Cow<VirtualFDEl>> m_Data;
        std::unordered_set<osc::UID> m_Selection;
    };
}

class osc::FrameDefinitionTab::Impl final {
public:

    Impl(TabHost* parent) : m_Parent{std::move(parent)}
    {
    }

    UID getID() const
    {
        return m_ID;
    }

    CStringView getName() const
    {
        return m_Name;
    }

    TabHost* parent()
    {
        return m_Parent;
    }

    void onMount()
    {

    }

    void onUnmount()
    {

    }

    bool onEvent(SDL_Event const&)
    {
        return false;
    }

    void onTick()
    {

    }

    void onDrawMainMenu()
    {

    }

    void onDraw()
    {

    }

private:
    // tab state
    UID m_ID;
    std::string m_Name = ICON_FA_COOKIE " FrameDefinitionTab";
    TabHost* m_Parent;
};


// public API (PIMPL)

osc::FrameDefinitionTab::FrameDefinitionTab(TabHost* parent) :
    m_Impl{std::make_unique<Impl>(std::move(parent))}
{
}

osc::FrameDefinitionTab::FrameDefinitionTab(FrameDefinitionTab&&) noexcept = default;
osc::FrameDefinitionTab& osc::FrameDefinitionTab::operator=(FrameDefinitionTab&&) noexcept = default;
osc::FrameDefinitionTab::~FrameDefinitionTab() noexcept = default;

osc::UID osc::FrameDefinitionTab::implGetID() const
{
    return m_Impl->getID();
}

osc::CStringView osc::FrameDefinitionTab::implGetName() const
{
    return m_Impl->getName();
}

osc::TabHost* osc::FrameDefinitionTab::implParent() const
{
    return m_Impl->parent();
}

void osc::FrameDefinitionTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::FrameDefinitionTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::FrameDefinitionTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::FrameDefinitionTab::implOnTick()
{
    m_Impl->onTick();
}

void osc::FrameDefinitionTab::implOnDrawMainMenu()
{
    m_Impl->onDrawMainMenu();
}

void osc::FrameDefinitionTab::implOnDraw()
{
    m_Impl->onDraw();
}
