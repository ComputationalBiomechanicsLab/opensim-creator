#include "FrameDefinitionTab.hpp"

#include "src/Utils/Assertions.hpp"
#include "src/Utils/CStringView.hpp"
#include "src/Utils/ClonePtr.hpp"

#include <glm/vec3.hpp>
#include <IconsFontAwesome5.h>
#include <SDL_events.h>

#include <algorithm>
#include <cstddef>
#include <functional>
#include <map>
#include <optional>
#include <sstream>
#include <stack>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

namespace
{
    constexpr osc::CStringView c_TabStringID = "OpenSim/Experimental/FrameDefinition";
    constexpr char c_PathSeperator = '/';
    constexpr std::string::value_type c_NullChar = {};

    /**
    * Returns a normalized form of `path`. A normalized path string is
    * guaranteed to:
    *
    * - Not contain any *internal* or *trailing* relative elements (e.g.
    *   'a/../b').
    *
    *     - It may *start* with relative elements (e.g. '../a/b'), but only
    *       if the path is non-absolute (e.g. "/../a/b" is invalid)
    *
    * - Not contain any repeated separators (e.g. 'a///b' --> 'a/b')
    *
    * Any attempt to step above the root of the expression with '..' will
    * result in an exception being thrown (e.g. "a/../.." will throw).
    *
    * This method is useful for path traversal and path manipulation
    * methods, because the above ensures that (e.g.) paths can be
    * concatenated and split into individual elements using basic
    * string manipulation techniques.
    */
    std::string NormalizePathString(std::string path)
    {
        // pathEnd is guaranteed to be a NUL terminator since C++11
        char* pathBegin = &path[0];
        char* pathEnd = &path[path.size()];

        // helper: shift n chars starting at newStart+n such that, after,
        // newStart..end is equal to what newStart+n..end was before.
        auto shift = [&](char* newStart, size_t n)
        {
            std::copy(newStart + n, pathEnd, newStart);
            pathEnd -= n;
        };

        // helper: grab 3 lookahead chars, using NUL as a senteniel to
        // indicate "past the end of the content".
        //
        // - The maximum lookahead is 3 characters because the parsing
        //   code below needs to be able to detect the upcoming input
        //   pattern "..[/\0]"
        struct Lookahead { char a, b, c; };
        auto getLookahead = [](char* start, char* end)
        {
            return Lookahead
            {
                start < end - 0 ? start[0] : c_NullChar,
                start < end - 1 ? start[1] : c_NullChar,
                start < end - 2 ? start[2] : c_NullChar,
            };
        };

        // remove duplicate adjacent separators
        for (char* c = pathBegin; c != pathEnd; ++c)
        {
            Lookahead l = getLookahead(c, pathEnd);
            if (l.a ==  c_PathSeperator && l.b == c_PathSeperator)
            {
                shift(c--, 1);
            }
        }

        bool isAbsolute = *pathBegin == c_PathSeperator;
        char* cursor = isAbsolute ? pathBegin + 1 : pathBegin;

        // skip/dereference relative elements *at the start of a path*
        Lookahead l = getLookahead(cursor, pathEnd);
        while (l.a == '.')
        {
            switch (l.b)
            {
            case c_PathSeperator:
                shift(cursor, 2);
                break;
            case c_NullChar:
                shift(cursor, 1);
                break;
            case '.':
            {
                if (l.c == c_PathSeperator || l.c == c_NullChar)
                {
                    // starts with '..' element: only allowed if the path
                    // is relative
                    if (isAbsolute)
                    {
                        throw std::runtime_error{path + ": is an invalid path: it is absolute, but starts with relative elements."};
                    }

                    // if not absolute, then make sure `contentStart` skips
                    // past these elements because the alg can't reduce
                    // them down
                    if (l.c == c_PathSeperator)
                    {
                        cursor += 3;
                    }
                    else
                    {
                        cursor += 2;
                    }
                }
                else
                {
                    // normal element that starts with '..'
                    ++cursor;
                }
                break;
            }
            default:
                // normal element that starts with '.'
                ++cursor;
                break;
            }

            l = getLookahead(cursor, pathEnd);
        }

        char* contentStart = cursor;

        // invariants:
        //
        // - the root path element (if any) has been skipped
        // - `contentStart` points to the start of the non-relative content of
        //   the supplied path string
        // - `path` contains no duplicate adjacent separators
        // - `[0..offset]` is normalized path string, but may contain a
        //   trailing slash
        // - `[contentStart..offset] is the normalized *content* of the path
        //   string

        while (cursor < pathEnd)
        {
            l = getLookahead(cursor, pathEnd);

            if (l.a == '.' && (l.b == c_NullChar || l.b == c_PathSeperator))
            {
                // handle '.' (if found)
                size_t charsInCurEl = l.b == c_PathSeperator ? 2 : 1;
                shift(cursor, charsInCurEl);

            }
            else if (l.a == '.' && l.b == '.' && (l.c == c_NullChar || l.c == c_PathSeperator))
            {
                // handle '..' (if found)

                if (cursor == contentStart)
                {
                    throw std::runtime_error{path + ": cannot handle '..' element in a path string: dereferencing this would hop above the root of the path."};
                }

                // search backwards for previous separator
                char* prevSeparator = cursor - 2;
                while (prevSeparator > contentStart && *prevSeparator != c_PathSeperator)
                {
                    --prevSeparator;
                }

                char* prevStart = prevSeparator <= contentStart ? contentStart : prevSeparator + 1;
                size_t charsInCurEl = (l.c == c_PathSeperator) ? 3 : 2;
                size_t charsInPrevEl = cursor - prevStart;

                cursor = prevStart;
                shift(cursor, charsInPrevEl + charsInCurEl);

            }
            else
            {
                // non-relative element: skip past the next separator or end
                cursor = std::find(cursor, pathEnd, c_PathSeperator) + 1;
            }
        }

        // edge case:
        // - There was a trailing slash in the input and, post reduction, the output
        //   string is only a slash. However, the input path wasnt initially an
        //   absolute path, so the output should be "", not "/"
        {
            char* beg = isAbsolute ? pathBegin + 1 : pathBegin;
            if (pathEnd - beg > 0 && pathEnd[-1] == c_PathSeperator)
            {
                --pathEnd;
            }
        }

        // resize output to only contain the normalized range
        path.resize(pathEnd - pathBegin);

        return path;
    }
}

namespace osc::ps
{
    // COMPONENT PATH
    //
    // - a normalized (i.e. ../x/.. --> ..) path string
    // - that encodes a path from a source component to a destination
    //   component (e.g. ../to/destination)
    // - where the path may be "absolute", which is a special encoding
    //   that tells the implementation that the source component must
    //   be the root of the component tree (e.g. /path/from/root/to/destination)
    class ComponentPath final {
    public:
        [[nodiscard]] static inline constexpr char delimiter() noexcept
        {
            return c_PathSeperator;
        }

        explicit ComponentPath(std::string_view str) :
            m_NormalizedPath{NormalizePathString(std::string{str})}
        {
        }

        operator CStringView () const
        {
            return m_NormalizedPath;
        }

        operator std::string_view () const
        {
            return m_NormalizedPath;
        }
    private:
        std::string m_NormalizedPath;
    };

    bool IsAbsolute(ComponentPath const& path)
    {
        std::string_view const sv{path};
        return !sv.empty() && sv.front() == '/';
    }

    // SOCKET
    //
    // - a directed graph edge
    // - FROM one component TO another component in the same component tree

    // SOCKET forward decls.
    class AbstractSocket;
    class Component;
    using ComponentMemberOffset = uint16_t;
    CStringView GetName(
        Component const&
    );
    void RegisterSocketInParent(
        Component& parent,
        AbstractSocket const&,
        ComponentMemberOffset
    );
    Component const* TryFindComponent(
        Component const&,
        ComponentPath const&
    );
    Component* TryFindComponentMut(
        Component&,
        ComponentPath const&
    );

    // type-erased base class for a socket
    class AbstractSocket {
    protected:
        AbstractSocket() = default;
        AbstractSocket(AbstractSocket const&) = default;
        AbstractSocket(AbstractSocket&&) noexcept = default;
        AbstractSocket& operator=(AbstractSocket const&) = default;
        AbstractSocket& operator=(AbstractSocket&&) noexcept = default;
    public:
        virtual ~AbstractSocket() noexcept = default;

        ComponentPath const& getConnecteePath() const
        {
            return implGetConnecteePath();
        }

        Component const* tryGetConnectee() const
        {
            return implTryGetConnectee();
        }

        Component* tryUpdConnectee()
        {
            return implTryUpdConnectee();
        }

        Component const& getOwner() const
        {
            return implGetOwner();
        }

        Component& updOwner()
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

    private:
        virtual ComponentPath const& implGetConnecteePath() const = 0;
        virtual Component const* implTryGetConnectee() const = 0;
        virtual Component* implTryUpdConnectee() = 0;
        virtual Component const& implGetOwner() const = 0;
        virtual Component& implUpdOwner() = 0;
        virtual CStringView implGetName() const = 0;
        virtual CStringView implGetDescription() const = 0;
    };

    // typed base class for a socket
    template<typename TConnectee>
    class Socket : public AbstractSocket {
    public:
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

    private:
        std::string createConnectionErrorMessage()
        {
            std::stringstream ss;
            ss << GetName(getOwner()) << ": cannot connect to " << getConnecteePath();
            return std::move(ss).str();
        }
    };

    // concrete class that defines a socket member in a component
    //
    // (used by `OSC_SOCKET`)
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
            SocketDefinition{std::string_view{}}
        {
        }

        SocketDefinition(std::string_view initialConnecteePath_) :
            m_ConnecteePath{initialConnecteePath_}
        {
            static_assert(FuncGetOffsetInParent() < std::numeric_limits<ComponentMemberOffset>::max(), "might not fit into component's offset table");
            RegisterSocketInParent(
                implUpdOwner(),
                *this,
                static_cast<ComponentMemberOffset>(FuncGetOffsetInParent())
            );
        }

    private:
        ComponentPath const& implGetConnecteePath() const final
        {
            return m_ConnecteePath;
        }

        Component const* implTryGetConnectee() const final
        {
            return TryFindComponent(implGetOwner(), m_ConnecteePath);
        }

        Component* implTryUpdConnectee() final
        {
            return TryFindComponentMut(implUpdOwner(), m_ConnecteePath);
        }

        Component const& implGetOwner() const final
        {
            static_assert(FuncGetOffsetInParent() < std::numeric_limits<ComponentMemberOffset>::max(), "might not fit into component's offset table");
            char const* thisMemLocation = reinterpret_cast<char const*>(this);
            return *reinterpret_cast<Component const*>(thisMemLocation - FuncGetOffsetInParent());
        }

        Component& implUpdOwner() final
        {
            static_assert(FuncGetOffsetInParent() < std::numeric_limits<ComponentMemberOffset>::max(), "might not fit into component's offset table");
            char* thisMemLocation = reinterpret_cast<char*>(this);
            return *reinterpret_cast<Component*>(thisMemLocation - FuncGetOffsetInParent());
        }

        CStringView implGetName() const final
        {
            return VName;
        }

        CStringView implGetDescription() const final
        {
            return VDescription;
        }

        ComponentPath m_ConnecteePath;
    };

    // PROPERTY
    //
    // - a single instance
    // - of a type selected from a compile-time set of simple types (e.g. float, string)
    // - that is always a direct member of a component class

    // PROPERTIES forward decls.
    class AbstractProperty;
    class Component;
    void RegisterPropertyInParent(
        Component& parent,
        AbstractProperty const&,
        ComponentMemberOffset
    );

    // the type of the property
    enum class PropertyType {
        Float = 0,
        Vec3,
        String,

        TOTAL,
    };
    static constexpr size_t NumPropertyTypes() noexcept
    {
        return static_cast<size_t>(PropertyType::TOTAL);
    }

    // compile-time metadata for each supported value type
    template<typename TValue>
    class PropertyMetadata;

    template<>
    class PropertyMetadata<float> final {
    public:
        static constexpr PropertyType type() { return PropertyType::Float; }
    };

    template<>
    class PropertyMetadata<glm::vec3> final {
    public:
        static constexpr PropertyType type() { return PropertyType::Vec3; }
    };

    template<>
    class PropertyMetadata<std::string> final {
    public:
        static constexpr PropertyType type() { return PropertyType::String; }
    };

    // type-erased base class for a property
    class AbstractProperty {
    protected:
        AbstractProperty() = default;
        AbstractProperty(AbstractProperty const&) = default;
        AbstractProperty(AbstractProperty&&) noexcept = default;
        AbstractProperty& operator=(AbstractProperty const&) = default;
        AbstractProperty& operator=(AbstractProperty&&) noexcept = default;
    public:
        virtual ~AbstractProperty() noexcept = default;

        Component const& getOwner() const
        {
            return implGetOwner();
        }

        Component& updOwner()
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

    private:
        virtual Component const& implGetOwner() const = 0;
        virtual Component& implUpdOwner() = 0;
        virtual CStringView implGetName() const = 0;
        virtual CStringView implGetDescription() const = 0;
        virtual PropertyType implGetPropertyType() const = 0;
    };

    // typed base class for a property
    template<typename TValue>
    class Property : public AbstractProperty {
    public:
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

    // concrete class that defines a property member in a component
    //
    // (used by `OSC_PROPERTY`)
    template<
        typename TParent,
        typename TValue,
        auto FuncGetOffsetInParent,
        char const* VName,
        char const* VDescription
    >
    class PropertyDefinition final : public Property<TValue> {
    public:
        PropertyDefinition() :
            PropertyDefinition{TValue{}}
        {
        }

        PropertyDefinition(TValue initialValue_) :
            m_Value{std::move(initialValue_)}
        {
            static_assert(FuncGetOffsetInParent() < std::numeric_limits<ComponentMemberOffset>::max(), "might not fit into component's offset table");
            RegisterPropertyInParent(
                implUpdOwner(),
                *this,
                static_cast<ComponentMemberOffset>(FuncGetOffsetInParent())
            );
        }

    private:
        Component const& implGetOwner() const final
        {
            static_assert(FuncGetOffsetInParent() < std::numeric_limits<ComponentMemberOffset>::max(), "might not fit into component's offset table");
            char const* thisMemLocation = reinterpret_cast<char const*>(this);
            return *reinterpret_cast<Component const*>(thisMemLocation - FuncGetOffsetInParent());
        }

        Component& implUpdOwner() final
        {
            static_assert(FuncGetOffsetInParent() < std::numeric_limits<ComponentMemberOffset>::max(), "might not fit into component's offset table");
            char* thisMemLocation = reinterpret_cast<char*>(this);
            return *reinterpret_cast<Component*>(thisMemLocation - FuncGetOffsetInParent());
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
            return PropertyMetadata<TValue>::type();
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


    // COMPONENT
    //
    // - a named object
    // - that may have a (component) parent
    // - and may own:
    //
    //   - properties (simple values)
    //   - sockets (graph edges)
    //   - subcomponents (children)

    class Component {
    protected:
        Component() = default;

        Component(Component const& src) :
            m_Parent{nullptr},
            m_Name{src.m_Name},
            m_DeclarationOrderedPropertyOffsets{src.m_DeclarationOrderedPropertyOffsets},
            m_DeclarationOrderedSocketOffsets{src.m_DeclarationOrderedSocketOffsets},
            m_LexographicallyOrderedSubcomponents{src.m_LexographicallyOrderedSubcomponents}
        {
            for (ClonePtr<Component> const& subcomponent : m_LexographicallyOrderedSubcomponents)
            {
                subcomponent->m_Parent = this;
            }
        }

        Component(Component&& tmp) noexcept :
            m_Parent{nullptr},
            m_Name{std::move(tmp.m_Name)},
            m_DeclarationOrderedPropertyOffsets{std::move(tmp.m_DeclarationOrderedPropertyOffsets)},
            m_DeclarationOrderedSocketOffsets{std::move(tmp.m_DeclarationOrderedSocketOffsets)},
            m_LexographicallyOrderedSubcomponents{std::move(tmp.m_LexographicallyOrderedSubcomponents)}
        {
            for (ClonePtr<Component> const& subcomponent : m_LexographicallyOrderedSubcomponents)
            {
                subcomponent->m_Parent = this;
            }
        }

        Component& operator=(Component const&) = delete;
        Component& operator=(Component&&) noexcept = delete;
    public:
        virtual ~Component() noexcept = default;

        std::unique_ptr<Component> clone() const
        {
            return implClone();
        }

        Component const* tryGetParent() const
        {
            return m_Parent;
        }

        Component* tryUpdParent()
        {
            return m_Parent;
        }

        CStringView getName() const
        {
            return m_Name;
        }

        void setName(std::string_view newName);

        size_t getNumProperties() const
        {
            return m_DeclarationOrderedPropertyOffsets.size();
        }
        AbstractProperty const& getIthProperty(ptrdiff_t i) const
        {
            ComponentMemberOffset const offset = m_DeclarationOrderedPropertyOffsets.at(i);
            char const* memoryLocation = reinterpret_cast<char const*>(this) + offset;
            return *reinterpret_cast<AbstractProperty const*>(memoryLocation);
        }
        AbstractProperty& updIthProperty(ptrdiff_t i)
        {
            ComponentMemberOffset const offset = m_DeclarationOrderedPropertyOffsets.at(i);
            char* memoryLocation = reinterpret_cast<char*>(this) + offset;
            return *reinterpret_cast<AbstractProperty*>(memoryLocation);
        }

        size_t getNumSockets() const
        {
            return m_DeclarationOrderedSocketOffsets.size();
        }
        AbstractSocket const& getIthSocket(ptrdiff_t i) const
        {
            ComponentMemberOffset const offset = m_DeclarationOrderedSocketOffsets.at(i);
            char const* memoryLocation = reinterpret_cast<char const*>(this) + offset;
            return *reinterpret_cast<AbstractSocket const*>(memoryLocation);
        }
        AbstractSocket& updIthSocket(ptrdiff_t i)
        {
            ComponentMemberOffset const offset = m_DeclarationOrderedSocketOffsets.at(i);
            char* memoryLocation = reinterpret_cast<char*>(this) + offset;
            return *reinterpret_cast<AbstractSocket*>(memoryLocation);
        }

        size_t getNumSubcomponents() const
        {
            return m_LexographicallyOrderedSubcomponents.size();
        }
        Component const& getIthSubcomponent(ptrdiff_t i) const
        {
            return *m_LexographicallyOrderedSubcomponents.at(i);
        }
        Component& updIthSubcomponent(ptrdiff_t i)
        {
            return *m_LexographicallyOrderedSubcomponents.at(i);
        }
        Component const* tryGetSubcomponentByName(std::string_view name) const
        {
            auto const begin = m_LexographicallyOrderedSubcomponents.begin();
            auto const end = m_LexographicallyOrderedSubcomponents.end();
            auto const lexographicallyLowerThanName = [name](osc::ClonePtr<Component> const& c)
            {
                return c->getName() < name;
            };
            auto const it = std::partition_point(begin, end, lexographicallyLowerThanName);
            return (it != end && (*it)->getName() == name) ? it->get() : nullptr;
        }

    private:
        friend class ComponentList;
        friend void RegisterSocketInParent(
            Component& parent,
            AbstractSocket const&,
            ComponentMemberOffset
        );
        friend void RegisterPropertyInParent(
            Component& parent,
            AbstractProperty const&,
            ComponentMemberOffset
        );
        friend Component const* TryFindComponent(
            Component const&,
            ComponentPath const&
        );

        virtual std::unique_ptr<Component> implClone() const = 0;

        Component* m_Parent = nullptr;
        std::string m_Name;
        std::vector<ComponentMemberOffset> m_DeclarationOrderedPropertyOffsets;
        std::vector<ComponentMemberOffset> m_DeclarationOrderedSocketOffsets;
        std::vector<ClonePtr<Component>> m_LexographicallyOrderedSubcomponents;
    };

    // type-erased component list container
    class ComponentList : public Component {
    };

    template<typename TComponent>
    class TypedComponentList : public ComponentList {
        void append(std::unique_ptr<TComponent> component);
    };

    Component const& GetRoot(Component const& component)
    {
        Component const* rv = &component;
        while (Component const* parent = rv->tryGetParent())
        {
            rv = parent;
        }
        return *rv;
    }

    class ComponentIterator final {
    public:
        ComponentIterator() = default;

        ComponentIterator(Component& c)
        {
            m_VisitorStack.emplace(&c, -1);
        }

        Component& operator*() noexcept
        {
            return *m_VisitorStack.top().component;
        }

        Component* operator->() noexcept
        {
            return m_VisitorStack.top().component;
        }

        ComponentIterator& operator++()
        {
            StackEl& top = m_VisitorStack.top();

            ++top.pos;
            if (top.pos < static_cast<ptrdiff_t>(top.component->getNumSubcomponents()))
            {
                m_VisitorStack.emplace(&top.component->updIthSubcomponent(top.pos), -1);
            }
            else
            {
                m_VisitorStack.pop();
            }
            return *this;
        }

    private:
        friend bool operator!=(ComponentIterator const&, ComponentIterator const&) noexcept;

        struct StackEl final {
            StackEl(Component* component_, ptrdiff_t pos_) :
                component{component_},
                pos{pos_}
            {
            }

            bool operator==(StackEl const& other) const noexcept
            {
                return component == other.component && pos == other.pos;
            }

            Component* component;
            ptrdiff_t pos;
        };

        std::stack<StackEl, std::vector<StackEl>> m_VisitorStack;
    };

    bool operator!=(ComponentIterator const& a, ComponentIterator const& b) noexcept
    {
        return a.m_VisitorStack != b.m_VisitorStack;
    }

    bool Contains(CStringView str, CStringView substr)
    {
        auto const it = std::search(
            str.begin(),
            str.end(),
            substr.begin(),
            substr.end()
        );
        return it != str.end();
    }

    void Component::setName(std::string_view newName)
    {
        if (!tryGetParent() || !dynamic_cast<ComponentList const*>(tryGetParent()))
        {
            return;  // can only rename a component that's within a component list
        }

        CStringView const oldName = this->getName();
        for (auto it = ComponentIterator{*this}; it != ComponentIterator{}; ++it)
        {
            for (size_t i = 0, nSockets = it->getNumSockets(); i < nSockets; ++i)
            {
                std::string_view const path = it->getIthSocket(i).getConnecteePath();
                auto elBegin = path.begin();
                while (elBegin != path.end())
                {
                    auto elEnd = std::find(elBegin, path.end(), ComponentPath::delimiter());
                    std::string_view const el(&*elBegin, elEnd - elBegin);
                    // TODO: rename path fragments etc.
                }
            }
        }
    }

    void RegisterSocketInParent(
        Component& parent,
        AbstractSocket const&,
        ComponentMemberOffset offset)
    {
        parent.m_DeclarationOrderedSocketOffsets.push_back(offset);
    }
    Component const* TryFindComponent(
        Component const& component,
        ComponentPath const& path)
    {
        std::string_view const pathSV = path;

        Component const* c = IsAbsolute(path) ? &GetRoot(component) : &component;
        auto elBegin = pathSV.begin();
        while (c && elBegin != pathSV.end())
        {
            auto const elEnd = std::find(elBegin, pathSV.end(), ComponentPath::delimiter());
            std::string_view const el(&*elBegin, elEnd - elBegin);

            c = c->tryGetSubcomponentByName(el);
            elBegin = elEnd;
        }

        return c;
    }
    Component* TryFindComponentMut(
        Component& component,
        ComponentPath const& path)
    {
        return const_cast<Component*>(TryFindComponent(component, path));
    }

    void RegisterPropertyInParent(
        Component& parent,
        AbstractProperty const&,
        ComponentMemberOffset offset)
    {
        parent.m_DeclarationOrderedPropertyOffsets.push_back(offset);
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
    > name

#define OSC_PROPERTY(ValueType, name, description) \
    static constexpr char _property_##name##_name[] = #name; \
    static constexpr char _property_##name##_description[] = #description;  \
    static constexpr size_t _property_##name##_offset_getter() \
    { \
        return offsetof(Self, name); \
    } \
    osc::ps::PropertyDefinition< \
        Self, \
        ValueType, \
        _property_##name##_offset_getter, \
        _property_##name##_name, \
        _property_##name##_description \
    > name

namespace osc::ps
{
    class SomeSubcomponent : public Component {
        OSC_COMPONENT(SomeSubcomponent);
    };

    class Sphere : public Component {
        OSC_COMPONENT(Sphere);
    private:
        OSC_PROPERTY(float, radius, "the radius of the sphere");
        OSC_PROPERTY(std::string, humanReadableName, "human readable name of the sphere");
        OSC_PROPERTY(glm::vec3, position, "the position of the point in 3D space");

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
