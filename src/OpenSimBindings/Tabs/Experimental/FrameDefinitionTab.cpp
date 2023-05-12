#include "FrameDefinitionTab.hpp"

#include "src/Utils/Assertions.hpp"
#include "src/Utils/CStringView.hpp"
#include "src/Utils/ClonePtr.hpp"

#include <glm/vec3.hpp>
#include <IconsFontAwesome5.h>
#include <SDL_events.h>

#include <cstddef>
#include <functional>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

namespace
{
    constexpr osc::CStringView c_TabStringID = "OpenSim/Experimental/FrameDefinition";
}

namespace osc::ps
{
    class Component;
    using ComponentMemberOffset = uint16_t;

    // SOCKET

    // type-erased socket base class
    class AbstractSocket {
    protected:
        AbstractSocket(
            Component& owner_,
            std::string_view initialConnecteePath_);
        AbstractSocket(AbstractSocket const&) = default;
        AbstractSocket(AbstractSocket&&) noexcept = default;
        AbstractSocket& operator=(AbstractSocket const&) = default;
        AbstractSocket& operator=(AbstractSocket&&) noexcept = default;
    public:
        virtual ~AbstractSocket() noexcept = default;

        CStringView getConnecteePath() const
        {
            return m_ConnecteePath;
        }

        bool isConnected() const
        {
            return tryGetConnectee() != nullptr;
        }

        virtual Component const& getOwner() const = 0;
        virtual Component& updOwner() = 0;
        virtual CStringView getName() const = 0;
        virtual CStringView getDescription() const = 0;

    protected:
        Component const* tryGetConnectee() const;
        Component* tryUpdConnectee();
        std::string createConnectionErrorMessage() const;

    private:
        std::string m_ConnecteePath;
    };

    // concretely typed, simple-to-downcast, socket
    //
    // this middle-level class between `AbstractSocket` and `SocketDefinition` exists
    // so that downstream code can be written as:
    //
    //     - ISocket& v = f(); dynamic_cast<Socket<T>&>(v);
    //
    // without having to know the more complicated permutation of template magic involved
    // with `SocketDefinition<T, U, OmgWhenWillThisEnd, ...>`
    template<typename TConnectee>
    class Socket : public AbstractSocket {
    public:
        using AbstractSocket::AbstractSocket;

        TConnectee const* tryGetConnectee() const
        {
            return dynamic_cast<TConnectee const*>(static_cast<AbstractSocket const&>(*this).tryGetConnectee());
        }

        TConnectee* tryUpdConnectee()
        {
            return dynamic_cast<TConnectee*>(static_cast<AbstractSocket&>(*this).tryUpdConnectee());
        }

        TConnectee const& getConnectee() const
        {
            TConnectee const* connectee = tryGetConnectee();
            if (connectee)
            {
                return *connectee;
            }
            else
            {
                throw std::runtime_error{createConnectionErrorMessage()};
            }
        }

        TConnectee& updConnectee()
        {
            TConnectee const* connectee = tryUpdConnectee();
            if (connectee)
            {
                return *connectee;
            }
            else
            {
                throw std::runtime_error{createConnectionErrorMessage()};
            }
        }
    };

    // concrete class that fully implements the `Socket` and `AbstractSocket` APIs
    //
    // this is what is ultimately owned by components with sockets. The reason it is
    // templated to high-heaven is to reduce the number of member variables each
    // instance holds
    template<
        typename TParent,
        typename TConnectee,
        auto FuncGetOffsetInParent,
        char const* VName,
        char const* VDescription
    >
    class SocketDefinition final : public Socket<TConnectee> {
    public:
        SocketDefinition() :
            Socket<TConnectee>(updOwner(), "")
        {
        }

        SocketDefinition(std::string_view initialConnecteePath_) :
            Socket<TConnectee>{updOwner(), initialConnecteePath_}
        {
        }

        Component const& getOwner() const final
        {
            static_assert(FuncGetOffsetInParent() < std::numeric_limits<ComponentMemberOffset>::max(), "might not fit into component's offset table");
            char const* thisMemLocation = reinterpret_cast<char const*>(this);
            return *reinterpret_cast<Component const*>(thisMemLocation - FuncGetOffsetInParent());
        }

        Component& updOwner() final
        {
            static_assert(FuncGetOffsetInParent() < std::numeric_limits<ComponentMemberOffset>::max(), "might not fit into component's offset table");
            char* thisMemLocation = reinterpret_cast<char*>(this);
            return *reinterpret_cast<Component*>(thisMemLocation - FuncGetOffsetInParent());
        }

        CStringView getName() const final
        {
            return VName;
        }

        CStringView getDescription() const final
        {
            return VDescription;
        }
    };


    // PROPERTIES

    // there are two component types: singular, which own properties/sockets,
    // and list, which own `n` subcomponents but have no properties/sockets
    //
    // the two are type-erased under `AbstractComponent` for traversal
    class AbstractComponent;

    // used for runtime checking of the property type
    enum class PropertyType {
        Float = 0,
        Vec3,
        String,
        Subcomponent,

        TOTAL,
    };
    static constexpr size_t NumPropertyTypes() noexcept
    {
        return static_cast<size_t>(PropertyType::TOTAL);
    }

    // this class must be specialized for each concrete property type that
    // is supported by the property system
    template<typename TValue>
    class SimplePropertyValueMetadata;

    template<>
    class SimplePropertyValueMetadata<float> final {
    public:
        static constexpr PropertyType type() { return PropertyType::Float; }
    };

    template<>
    class SimplePropertyValueMetadata<glm::vec3> final {
    public:
        static constexpr PropertyType type() { return PropertyType::Vec3; }
    };

    template<>
    class SimplePropertyValueMetadata<std::string> final {
    public:
        static constexpr PropertyType type() { return PropertyType::String; }
    };

    // type-erased property base class
    class AbstractProperty {
    protected:
        AbstractProperty() = default;  // auto-registration isn't needed for the list component
        explicit AbstractProperty(Component& owner_);
        AbstractProperty(AbstractProperty const&) = default;
        AbstractProperty(AbstractProperty&&) noexcept = default;
        AbstractProperty& operator=(AbstractProperty const&) = default;
        AbstractProperty& operator=(AbstractProperty&&) noexcept = default;
    public:
        virtual ~AbstractProperty() noexcept = default;

        AbstractComponent const& getOwner() const
        {
            return implGetOwner();
        }

        AbstractComponent& updOwner()
        {
            return implUpdOwner();
        }

        CStringView getName() const
        {
            return implGetName();
        }

        CStringView getDescription() const
        {
            return implGetDescription();
        }

        PropertyType getPropertyType() const
        {
            return implGetPropertyType();
        }

        size_t getNumValues() const
        {
            return implGetNumValues();
        }

        AbstractComponent const& getValueAsAbstractComponent() const
        {
            return implGetValueAsAbstractComponent();
        }

        AbstractComponent& updValueAsAbstractComponent()
        {
            return implUpdValueAsAbstractComponent();
        }

    private:
        virtual AbstractComponent const& implGetOwner() const = 0;
        virtual AbstractComponent& implUpdOwner() = 0;
        virtual CStringView implGetName() const = 0;
        virtual CStringView implGetDescription() const = 0;
        virtual PropertyType implGetPropertyType() const = 0;
        virtual size_t implGetNumValues() const = 0;
        virtual AbstractComponent const& implGetValueAsAbstractComponent() const = 0;
        virtual AbstractComponent& implUpdValueAsAbstractComponent() = 0;
    };

    template<typename TValue>
    class Property : public AbstractProperty {
    public:
        using AbstractProperty::AbstractProperty;

        TValue const& getValue() const
        {
            return implGetValue();
        }

        TValue& updValue()
        {
            return implUpdValue();
        }

        TValue const* operator->() const
        {
            return &implGetValue();
        }

        TValue* operator->()
        {
            return &implUpdValue();
        }

        TValue const& operator*() const
        {
            return implGetValue();
        }

        TValue& operator*()
        {
            return implUpdValue();
        }

    private:
        virtual TValue const& implGetValue() const = 0;
        virtual TValue& implUpdValue() = 0;
    };

    template<
        typename TParent,
        typename TValue,
        auto FuncGetOffsetInParent,
        char const* VName,
        char const* VDescription
    >
    class SingleSimpleProperty : public Property<TValue> {
    public:
        SingleSimpleProperty() :
            Property<TValue>(implUpdOwner()),
            m_Value{}
        {
        }

        SingleSimpleProperty(TValue initialValue_) :
            Property<TValue>{implUpdOwner()},
            m_Value{std::move(initialValue_)}
        {
        }

    private:
        AbstractComponent const& implGetOwner() const final
        {
            static_assert(FuncGetOffsetInParent() < std::numeric_limits<ComponentMemberOffset>::max(), "might not fit into component's offset table");
            char const* thisMemLocation = reinterpret_cast<char const*>(this);
            return *reinterpret_cast<AbstractComponent const*>(thisMemLocation - FuncGetOffsetInParent());
        }

        AbstractComponent& implUpdOwner() final
        {
            static_assert(FuncGetOffsetInParent() < std::numeric_limits<ComponentMemberOffset>::max(), "might not fit into component's offset table");
            char* thisMemLocation = reinterpret_cast<char*>(this);
            return *reinterpret_cast<AbstractComponent*>(thisMemLocation - FuncGetOffsetInParent());
        }

        CStringView implGetName() const final
        {
            return VName;
        }

        CStringView implGetDescription() const final
        {
            return VDescription;
        }

        PropertyType implGetPropertyType() const final
        {
            return SimplePropertyValueMetadata<TValue>::type();
        }

        size_t implGetNumValues() const final
        {
            return 1;
        }

        AbstractComponent const& implGetValueAsAbstractComponent() const final
        {
            std::stringstream ss;
            ss << implGetName() << ": getUpdValueAsObject was called on a simple property: this is not allowed (and is probably a developer error)";
            throw std::runtime_error{std::move(ss).str()};
        }

        AbstractComponent& implUpdValueAsAbstractComponent() final
        {
            std::stringstream ss;
            ss << implGetName() << ": implUpdValueAsObject was called on a simple property: this is not allowed (and is probably a developer error)";
            throw std::runtime_error{std::move(ss).str()};
        }

        TValue const& implGetValue() const final
        {
            return m_Value;
        }

        TValue& implUpdValue() final
        {
            return m_Value;
        }

        TValue m_Value;
    };

    template<
        typename TParent,
        typename TComponent,
        auto FuncGetOffsetInParent,
        char const* VName,
        char const* VDescription
    >
    class SubcomponentProperty final : public Property<TComponent> {
    public:
        SubcomponentProperty() :
            Property<TComponent>(implUpdOwner()),
            m_Component{std::make_unique<TComponent>()}
        {
        }

    private:
        // TODO: this only works for static components, not components in a vector etc.
        AbstractComponent const& implGetOwner() const final
        {
            static_assert(FuncGetOffsetInParent() < std::numeric_limits<ComponentMemberOffset>::max(), "might not fit into component's offset table");
            char const* thisMemLocation = reinterpret_cast<char const*>(this);
            return *reinterpret_cast<AbstractComponent const*>(thisMemLocation - FuncGetOffsetInParent());
        }

        AbstractComponent& implUpdOwner() final
        {
            static_assert(FuncGetOffsetInParent() < std::numeric_limits<ComponentMemberOffset>::max(), "might not fit into component's offset table");
            char* thisMemLocation = reinterpret_cast<char*>(this);
            return *reinterpret_cast<AbstractComponent*>(thisMemLocation - FuncGetOffsetInParent());
        }

        CStringView implGetName() const final
        {
            return VName;
        }

        CStringView implGetDescription() const final
        {
            return VDescription;
        }

        PropertyType implGetPropertyType() const final
        {
            return PropertyType::Subcomponent;
        }

        size_t implGetNumValues() const final
        {
            return 1;
        }

        AbstractComponent const& implGetValueAsAbstractComponent() const final
        {
            return *m_Component;
        }

        AbstractComponent& implUpdValueAsAbstractComponent() final
        {
            return *m_Component;
        }

        TComponent const& implGetValue() const final
        {
            return *m_Component;
        }

        TComponent& implUpdValue() final
        {
            return *m_Component;
        }

        ClonePtr<TComponent> m_Component;
    };


    // COMPONENT

    // base class for a "component", which is essentially a node in a property
    // tree of objects
    class AbstractComponent {
    protected:
        AbstractComponent() = default;
        AbstractComponent(AbstractComponent const&) = default;
        AbstractComponent(AbstractComponent&&) noexcept = default;
        AbstractComponent& operator=(AbstractComponent const&) = default;
        AbstractComponent& operator=(AbstractComponent&&) noexcept = default;
    public:
        virtual ~AbstractComponent() noexcept = default;

        AbstractComponent const& getParent() const
        {
            return implGetParent();
        }

        AbstractComponent& updParent()
        {
            return implUpdParent();
        }

        CStringView getName() const
        {
            return implGetName();
        }

        size_t getNumProperties() const
        {
            return implGetNumProperties();
        }

        AbstractProperty const& getIthProperty(ptrdiff_t i) const
        {
            return implGetIthProperty(i);
        }

        AbstractProperty& updIthProperty(ptrdiff_t i)
        {
            return implUpdIthProperty(i);
        }

    private:
        virtual AbstractComponent const& implGetParent() const;
        virtual AbstractComponent& implUpdParent();
        virtual CStringView implGetName() const;
        virtual size_t implGetNumProperties() const;
        virtual AbstractProperty const& implGetIthProperty(ptrdiff_t) const;
        virtual AbstractProperty& implUpdIthProperty(ptrdiff_t);
    };

    // type-erased base class for a "list component", which is a specialized
    // node that holds a dynamically-sized list of uniquely-named properties
    class DynamicComponent : public AbstractComponent {
    protected:
        DynamicComponent() = default;
        DynamicComponent(DynamicComponent const&) = default;
        DynamicComponent(DynamicComponent&&) noexcept = default;
        DynamicComponent& operator=(DynamicComponent const&) = default;
        DynamicComponent& operator=(DynamicComponent&&) noexcept = default;
    public:
        virtual ~DynamicComponent() noexcept = default;

        size_t size() const
        {
            return implSize();
        }

        AbstractComponent const& operator[](ptrdiff_t i) const
        {
            return implGetIthElement(i);
        }

        AbstractComponent& operator[](ptrdiff_t i)
        {
            return implUpdIthElement(i);
        }
    private:
        // a list node shouldn't have properties or sockets
        size_t implGetNumProperties() const final
        {
            return 0;
        }

        AbstractProperty const& implGetIthProperty(ptrdiff_t) const final
        {
            throw std::runtime_error{"tried to get a property from a list component: this is always an error"};
        }

        AbstractProperty& implUpdIthProperty(ptrdiff_t) final
        {
            throw std::runtime_error{"tried to get a property from a list component: this is always an error"};
        }

        virtual size_t implSize() const = 0;
        virtual AbstractComponent const& implGetIthElement(ptrdiff_t) const = 0;
        virtual AbstractComponent& implUpdIthElement(ptrdiff_t) = 0;
    };

    template<typename TSubcomponent>
    class ListComponent final : AbstractListComponent {
    private:
        std::vector<ClonePtr<TSubcomponent>> m_Components;
    };

    // a normal "singular" component that contains a sequence of properties
    // and sockets
    class Component {
    protected:
        // TODO: wipe parent pointers etc.
        Component() = default;
        Component(Component const&) = default;
        Component(Component&&) = default;
        Component& operator=(Component const&) = default;
        Component& operator=(Component&&) noexcept = default;
    public:
        virtual ~Component() noexcept = default;

        CStringView getName() const
        {
            return m_Name;
        }

        std::unique_ptr<Component> clone() const
        {
            return implClone();
        }
    private:
        friend class AbstractSocket;
        friend class AbstractProperty;

        virtual AbstractComponent const& implGetParent() const;
        virtual AbstractComponent& implUpdParent();
        virtual CStringView implGetName() const;
        virtual size_t implGetNumProperties() const;
        virtual AbstractProperty const& implGetIthProperty(ptrdiff_t) const;
        virtual AbstractProperty& implUpdIthProperty(ptrdiff_t);

        virtual std::unique_ptr<Component> implClone() const = 0;

        enum MemberFlags : int16_t {
            MemberFlags_IsProperty = 1<<0,
            MemberFlags_IsSocket = 1<<1,
            MemberFlags_IsLeafProperty = 1<<2,
            MemberFlags_IsSubcomponentProperty = 1<<3,
            MemberFlags_IsSubcomponentListProperty = 1<<4,
        };

        struct MemberMetadata final {
            MemberFlags flags;
            ComponentMemberOffset offset;
        };

        void registerSocket(AbstractSocket const& socket)
        {
            size_t offset = reinterpret_cast<char const*>(&socket) - reinterpret_cast<char const*>(this);
            OSC_ASSERT(offset <= std::numeric_limits<ComponentMemberOffset>::max());

            m_ComponentMemberMetadata.push_back(
                MemberMetadata
                {
                    MemberFlags_IsSocket,
                    static_cast<ComponentMemberOffset>(offset),
                }
            );

            sortMemberMetadata();
        }

        void registerProperty(AbstractProperty const& property)
        {
            size_t offset = reinterpret_cast<char const*>(&property) - reinterpret_cast<char const*>(this);
            OSC_ASSERT(offset <= std::numeric_limits<ComponentMemberOffset>::max());

            // TODO: check if property is a subcomponent or list of subcomponents and flag them
        }

        void sortMemberMetadata()
        {
            // TODO: sort by flags, then offset
        }

        std::weak_ptr<Component> m_MaybeParent;
        std::string m_Name;
        std::vector<MemberMetadata> m_ComponentMemberMetadata;
    };


    AbstractSocket::AbstractSocket(
        Component& owner_,
        std::string_view initialConnecteePath_) :

        m_ConnecteePath{std::move(initialConnecteePath_)}
    {
        owner_.registerSocket(*this);
    }

    Component const* AbstractSocket::tryGetConnectee() const
    {
        return nullptr;  // todo
    }

    Component* AbstractSocket::tryUpdConnectee()
    {
        return nullptr;
    }

    std::string AbstractSocket::createConnectionErrorMessage() const
    {
        std::stringstream ss;
        ss << getName() << ": cannot connect to connectee (" << getConnecteePath() << ')';
        return std::move(ss).str();
    }

    AbstractProperty::AbstractProperty(Component& owner_)
    {
        owner_.registerProperty(*this);
    }
}

#include <cstddef>
#define OSC_COMPONENT(ClassType) \
private: \
    using Self = ClassType; \
public: \
    std::unique_ptr<ClassType> clone() const \
    { \
        return std::make_unique<ClassType>(*this); \
    } \
private: \
    std::unique_ptr<osc::ps::Component> implClone() const final \
    { \
        return clone(); \
    }

#define OSC_SOCKET(ConnecteeType, name, description) \
    static constexpr char _property_##name##_name[] = #name; \
    static constexpr char _property_##name##_description[] = #description;  \
    static constexpr size_t _property_##name##_offset_getter() \
    { \
        return offsetof(Self, name); \
    } \
    osc::ps::SocketDefinition< \
        Self, \
        ConnecteeType, \
        _property_##name##_offset_getter, \
        _property_##name##_name, \
        _property_##name##_description \
    > name;

#define OSC_PROPERTY(ValueType, name, description) \
    static constexpr char _property_##name##_name[] = #name; \
    static constexpr char _property_##name##_description[] = #description;  \
    static constexpr size_t _property_##name##_offset_getter() \
    { \
        return offsetof(Self, name); \
    } \
    osc::ps::SingleSimpleProperty< \
        Self, \
        ValueType, \
        _property_##name##_offset_getter, \
        _property_##name##_name, \
        _property_##name##_description \
    > name;

#define OSC_COMPONENT_PROPERTY(ComponentType, name, description) \
    static constexpr char _property_##name##_name[] = #name; \
    static constexpr char _property_##name##_description[] = #description;  \
    static constexpr size_t _property_##name##_offset_getter() \
    { \
        return offsetof(Self, name); \
    } \
    static_assert(std::is_base_of_v<osc::ps::Component, ComponentType>, "component properties must inherit from Component"); \
    osc::ps::SubcomponentProperty< \
        Self, \
        ComponentType, \
        _property_##name##_offset_getter, \
        _property_##name##_name, \
        _property_##name##_description \
    > name;

#define OSC_COMPONENT_LIST_PROPERTY(ComponentType, name, description) \
    static constexpr char _property_##name##_name[] = #name; \
    static constexpr char _property_##name##_description[] = #description;  \
    static constexpr size_t _property_##name##_offset_getter() \
    { \
        return offsetof(Self, name); \
    } \
    static_assert(std::is_base_of_v<osc::ps::Component, ComponentType>, "component properties must inherit from Component"); \
    osc::ps::SubcomponentListProperty< \
        Self, \
        ComponentType, \
        _property_##name##_offset_getter, \
        _property_##name##_name, \
        _property_##name##_description \
    > name;

namespace osc::ps
{
    class SomeSubcomponent : public Component {
        OSC_COMPONENT(SomeSubcomponent);
    };

    class Sphere : public Component {
        OSC_COMPONENT(Sphere);
    private:
        OSC_PROPERTY(float, radius, "the radius of the sphere");
        OSC_PROPERTY(std::string, name, "name of the sphere");
        OSC_PROPERTY(glm::vec3, position, "the position of the point in 3D space");
        OSC_COMPONENT_PROPERTY(SomeSubcomponent, subcomponent, "the subcomponent");

        OSC_SOCKET(Sphere, sphere2sphere, "sphere to sphere connection");
    };
}

class osc::FrameDefinitionTab::Impl final {
public:

    Impl(std::weak_ptr<TabHost> parent_) :
        m_Parent{std::move(parent_)}
    {
        ps::Sphere s;
        ps::Sphere copy{s};
    }

    UID getID() const
    {
        return m_TabID;
    }

    CStringView getName() const
    {
        return c_TabStringID;
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
    UID m_TabID;
    std::weak_ptr<TabHost> m_Parent;
};


// public API (PIMPL)

osc::CStringView osc::FrameDefinitionTab::id() noexcept
{
    return c_TabStringID;
}

osc::FrameDefinitionTab::FrameDefinitionTab(std::weak_ptr<TabHost> parent_) :
    m_Impl{std::make_unique<Impl>(std::move(parent_))}
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
