#include "ShapeFittingTab.hpp"

#include <oscar/Graphics/Color.hpp>
#include <oscar/Graphics/Mesh.hpp>
#include <oscar/Maths/Ellipsoid.hpp>
#include <oscar/Maths/Plane.hpp>
#include <oscar/Maths/Sphere.hpp>
#include <oscar/UI/Tabs/StandardTabBase.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/UndoRedo.hpp>
#include <oscar/Utils/VariantHelpers.hpp>

#include <IconsFontAwesome5.h>
#include <SDL_events.h>

#include <cstddef>
#include <filesystem>
#include <limits>
#include <optional>
#include <map>
#include <string>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

namespace
{
    constexpr osc::CStringView c_TabStringID = "OpenSim/ShapeFitting";
}

// document-level concerns for the shape fitting UI
namespace shapefit
{
    // the value of a property in the shape fitting document
    class PropertyValue final {
    public:
        enum class Type {
            Float,
            Vec3,
        };

        explicit PropertyValue(float v_) :
            m_Storage{v_}
        {
        }

        explicit PropertyValue(glm::vec3 const& v_) :
            m_Storage{v_}
        {
        }

        Type type() const
        {
            Type rv = Type::Float;
            std::visit(osc::Overload
            {
                [&rv](float) { rv = Type::Float; },
                [&rv](glm::vec3 const&) { rv = Type::Vec3; },
            }, m_Storage);
            return rv;
        }

        float toFloat() const
        {
            float rv = std::numeric_limits<float>::quiet_NaN();
            std::visit(osc::Overload
            {
                [&rv](float v) { rv = v; },
                [](auto const&) {},
            }, m_Storage);
            return rv;
        }

        glm::vec3 toVec3() const
        {
            glm::vec3 rv = glm::vec3(std::numeric_limits<glm::vec3::value_type>::quiet_NaN());
            std::visit(osc::Overload
            {
                [&rv](glm::vec3 const& v) { rv = v; },
                [](auto const&) {},
            }, m_Storage);
            return rv;
        }
    private:
        std::variant<float, glm::vec3> m_Storage;
    };

    // a property in the shape fitting document
    class Property final {
    public:
        template<typename... TValueArgs>
        Property(std::string name, TValueArgs&&... args) :
            m_Name{std::move(name)},
            m_Value{std::forward<TValueArgs>(args)...}
        {
        }

        osc::CStringView name() const
        {
            return m_Name;
        }

        PropertyValue const& value() const
        {
            return m_Value;
        }
    private:
        std::string m_Name;
        PropertyValue m_Value;
    };

    // base class for "something in the shape fitting document"
    class IDocumentObject {
    protected:
        IDocumentObject() = default;
        IDocumentObject(IDocumentObject const&) = default;
        IDocumentObject(IDocumentObject&&) noexcept = default;
        IDocumentObject& operator=(IDocumentObject const&) = default;
        IDocumentObject& operator=(IDocumentObject&&) noexcept = default;
    public:
        virtual ~IDocumentObject() noexcept = default;

        std::string getLabel() const
        {
            return implGetLabel();
        }

        size_t getNumProperties() const
        {
            return implGetNumProperties();
        }

        std::optional<Property> getIthProperty(size_t i) const
        {
            return implGetIthProperty(i);
        }
    private:
        virtual std::string implGetLabel() const = 0;
        virtual size_t implGetNumProperties() const = 0;
        virtual std::optional<Property> implGetIthProperty(size_t) const = 0;
    };

    // returns a human-readable label for the ellipsoid
    std::string GetLabel(osc::Ellipsoid const&)
    {
        return "Ellipsoid";
    }

    // returns all properties of an ellipsoid
    std::vector<Property> GetProperties(osc::Ellipsoid const& e)
    {
        static_assert(std::tuple_size_v<decltype(osc::Ellipsoid::radiiDirections)> == 3);
        return
        {
            {"origin", e.origin},
            {"radii", e.radii},
            {"axis1", e.radiiDirections[0]},
            {"axis2", e.radiiDirections[1]},
            {"axis3", e.radiiDirections[2]},
        };
    }

    // returns a human-readable label for the plane
    std::string GetLabel(osc::Plane const&)
    {
        return "Plane";
    }

    // returns all properties of the plane
    std::vector<Property> GetProperties(osc::Plane const& p)
    {
        return
        {
            {"normal", p.normal},
            {"origin", p.origin},
        };
    }

    // returns a human-readable label for the sphere
    std::string GetLabel(osc::Sphere const&)
    {
        return "Sphere";
    }

    // returns all properties for the sphere
    std::vector<Property> GetProperties(osc::Sphere const& s)
    {
        return
        {
            {"origin", s.origin},
            {"radius", s.radius},
        };
    }

    // a shape fitting result (e.g. from fitting an ellipsoid to
    // mesh data)
    class ShapeFit final : public IDocumentObject {
    public:
        explicit ShapeFit(osc::Ellipsoid geom_) :
            m_Data{geom_}
        {
        }

        explicit ShapeFit(osc::Plane geom_) :
            m_Data{geom_}
        {
        }

        explicit ShapeFit(osc::Sphere sphere_) :
            m_Data{sphere_}
        {
        }

    private:
        std::string implGetLabel() const final
        {
            std::string rv;
            std::visit([&rv](auto const& v) { rv = GetLabel(v); }, m_Data);
            return rv;
        }

        size_t implGetNumProperties() const final
        {
            size_t rv = 0;
            std::visit([&rv](auto const& v) { rv = GetProperties(v).size(); }, m_Data);
            return rv;
        }

        std::optional<Property> implGetIthProperty(size_t i) const
        {
            std::optional<Property> rv;
            std::visit([&rv, i](auto const& v) { rv = GetProperties(v).at(i); }, m_Data);
            return rv;
        }

        std::variant<osc::Ellipsoid, osc::Plane, osc::Sphere> m_Data;
    };

    // a mesh, as displayed in the shape fitting UI
    class Mesh final : public IDocumentObject {
    public:
        Mesh(
            std::filesystem::path meshFilePath_,
            osc::Mesh mesh_) :
            m_MaybeMeshFilePath{std::move(meshFilePath_)},
            m_Mesh{std::move(mesh_)}
        {
        }
    private:
        std::string implGetLabel() const final
        {
            if (m_MaybeMeshFilePath)
            {
                return m_MaybeMeshFilePath->filename().string();
            }
            else
            {
                return "Mesh";
            }
        }

        size_t implGetNumProperties() const final
        {
            return 0;
        }

        std::optional<Property> implGetIthProperty(size_t) const
        {
            return std::nullopt;
        }

        std::optional<std::filesystem::path> m_MaybeMeshFilePath;
        osc::Mesh m_Mesh;
        std::map<osc::UID, ShapeFit> m_ShapeFits;
    };

    // a safe-to-copy, safe-to-move-around, associative "key" into a `Document`
    class DocumentObjectLookupKey final {
    public:
        DocumentObjectLookupKey() = default;

        DocumentObjectLookupKey(osc::UID meshID) :
            m_PathElements{meshID}
        {
        }

        DocumentObjectLookupKey(osc::UID meshID, osc::UID fitID) :
            m_PathElements{meshID, fitID}
        {
        }
    private:
        friend class Document;

        bool empty() const
        {
            return m_PathElements.empty();
        }

        size_t size() const
        {
            return m_PathElements.size();
        }

        osc::UID operator[](size_t i) const
        {
            return m_PathElements[i];
        }

        std::vector<osc::UID> m_PathElements;
    };

    // the logical shape fitting document that the user is editing via the UI
    class Document final {
    public:
        std::pair<osc::UID, Mesh&> insert(Mesh);
        IDocumentObject const* find(DocumentObjectLookupKey const&) const;
        IDocumentObject* find(DocumentObjectLookupKey const&);
        size_t erase(DocumentObjectLookupKey const&);

    private:
        std::map<osc::UID, Mesh> m_Content;
    };

    IDocumentObject const* Get(Document const&, DocumentObjectLookupKey const&)
    {
        return nullptr;  // TODO
    }
}

namespace shapefit::ui
{
    // document state that is required by the UI
    //
    // this is state that's only required by the UI, rather than being strictly
    // required for shape fitting - e.g. the only side-effects of changing this
    // only affect what (is not) shown by the UI
    class UIDocumentState final {
    private:
        DocumentObjectLookupKey m_Selection;
    };

    // the top-level document that the UI manipulates
    class UIDocument final {
    private:
        Document m_Document;
        UIDocumentState m_UIState;
    };

    // undoable version of the top-level document that the UI manipulates
    class UndoableUIDocument final {
    private:
        osc::UndoRedoT<UIDocument> m_Storage;
    };
}

class osc::ShapeFittingTab::Impl final : public osc::StandardTabBase {
public:
    Impl() : StandardTabBase{c_TabStringID}
    {
    }

private:
    void implOnMount() final
    {
    }

    void implOnUnmount() final
    {
    }

    bool implOnEvent(SDL_Event const&) final
    {
        return false;
    }

    void implOnTick() final
    {
    }

    void implOnDrawMainMenu() final
    {
    }

    void implOnDraw() final
    {
    }
};


// public API

osc::CStringView osc::ShapeFittingTab::id() noexcept
{
    return c_TabStringID;
}

osc::ShapeFittingTab::ShapeFittingTab(ParentPtr<TabHost> const&) :
    m_Impl{std::make_unique<Impl>()}
{
}

osc::ShapeFittingTab::ShapeFittingTab(ShapeFittingTab&&) noexcept = default;
osc::ShapeFittingTab& osc::ShapeFittingTab::operator=(ShapeFittingTab&&) noexcept = default;
osc::ShapeFittingTab::~ShapeFittingTab() noexcept = default;

osc::UID osc::ShapeFittingTab::implGetID() const
{
    return m_Impl->getID();
}

osc::CStringView osc::ShapeFittingTab::implGetName() const
{
    return m_Impl->getName();
}

void osc::ShapeFittingTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::ShapeFittingTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::ShapeFittingTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::ShapeFittingTab::implOnTick()
{
    m_Impl->onTick();
}

void osc::ShapeFittingTab::implOnDrawMainMenu()
{
    m_Impl->onDrawMainMenu();
}

void osc::ShapeFittingTab::implOnDraw()
{
    m_Impl->onDraw();
}
