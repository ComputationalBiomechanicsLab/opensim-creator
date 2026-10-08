#pragma once

#include <liboscar/concepts/dereferences_to.h>
#include <liboscar/graphics/color.h>
#include <liboscar/maths/ray.h>
#include <liboscar/maths/vector.h>
#include <liboscar/shims/cpp23/generator.h>
#include <liboscar/utilities/c_string_view.h>
#include <liboscar/utilities/string_name.h>
#include <OpenSim/Common/ComponentPath.h>
#include <SimTKcommon/internal/Transform.h>

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <filesystem>
#include <functional>
#include <iosfwd>
#include <iterator>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <string_view>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace OpenSim { class AbstractOutput; }
namespace OpenSim { class AbstractPathPoint; }
namespace OpenSim { class AbstractProperty; }
namespace OpenSim { class AbstractSocket; }
namespace OpenSim { class Appearance; }
namespace OpenSim { template<typename> class Array; }
namespace OpenSim { template<typename> class ArrayPtrs; }
namespace OpenSim { class Body; }
namespace OpenSim { class Component; }
namespace OpenSim { class ComponentPath; }
namespace OpenSim { class Constraint; }
namespace OpenSim { class Coordinate; }
namespace OpenSim { class ExternalForce; }
namespace OpenSim { class Frame; }
namespace OpenSim { class Geometry; }
namespace OpenSim { class GeometryPath; }
namespace OpenSim { class HuntCrossleyForce; }
namespace OpenSim { class Joint; }
namespace OpenSim { class Marker; }
namespace OpenSim { class Mesh; }
namespace OpenSim { class Model; }
namespace OpenSim { class ModelComponent; }
namespace OpenSim { class Muscle; }
namespace OpenSim { class Object; }
namespace OpenSim { template<typename> class Property; }
namespace OpenSim { template<typename> class ObjectProperty; }
namespace OpenSim { class PhysicalFrame; }
namespace OpenSim { class PhysicalOffsetFrame; }
namespace OpenSim { template<typename, typename> class Set; }
namespace OpenSim { template<typename> class SimpleProperty; }
namespace OpenSim { class Storage; }
namespace OpenSim { class WrapObject; }
namespace SimTK { class State; }
namespace SimTK { class SimbodyMatterSubsystem; }

// OpenSimHelpers: a collection of various helper functions that are used by `osc`
namespace opyn
{
    // Is satisfied if `T` has a `.clone()` member method that returns a raw `T*` pointer.
    template<typename T>
    concept ClonesToRawPointer = requires(const T& v) {
        { v.clone() } -> std::same_as<T*>;
    };

    // Returns the number of elements in `s` (see: `std::size`).
    template<
        std::derived_from<OpenSim::Object> T,
        std::derived_from<OpenSim::Object> C = OpenSim::Object
    >
    size_t size(const OpenSim::Set<T, C>& s)
    {
        return static_cast<size_t>(s.getSize());
    }

    // Returns the number of elements in `s` as a signed integer (see: `std::ssize`).
    template<
        std::derived_from<OpenSim::Object> T,
        std::derived_from<OpenSim::Object> C = OpenSim::Object
    >
    ptrdiff_t ssize(const OpenSim::Set<T, C>& s)
    {
        return static_cast<ptrdiff_t>(s.getSize());
    }

    // Returns the number of elements in `ary` (see: `std::size`).
    template<typename T>
    size_t size(const OpenSim::ArrayPtrs<T>& ary)
    {
        return static_cast<size_t>(ary.getSize());
    }

    // Returns the number of elements in `ary` (see: `std::size`).
    template<typename T>
    size_t size(const OpenSim::Array<T>& ary)
    {
        return static_cast<size_t>(ary.getSize());
    }

    // Returns the number of elements in `p` (see `std::size`).
    template<typename T>
    size_t size(const OpenSim::Property<T>& p)
    {
        return static_cast<size_t>(p.size());
    }

    // Returns an iterator to the beginning of `ary` (see: `std::begin`, `std::ranges::begin`).
    template<typename T>
    const T* begin(const OpenSim::Array<T>& ary)
    {
        return size(ary) != 0 ? std::addressof(ary[0]) : nullptr;
    }

    // Returns an iterator to the beginning of `ary` (see: `std::begin`, `std::ranges::begin`).
    template<typename T>
    T* begin(OpenSim::Array<T>& ary)
    {
        return size(ary) != 0 ? std::addressof(ary[0]) : nullptr;
    }

    // Returns an iterator to the end (i.e. the element after the last element) of `ary` (see: `std::end`, `std::ranges::end`)
    template<typename T>
    const T* end(const OpenSim::Array<T>& ary)
    {
        return begin(ary) + size(ary);
    }

    // Returns an iterator to the end (i.e. the element after the last element) of `ary` (see: `std::end`, `std::ranges::end`)
    template<typename T>
    T* end(OpenSim::Array<T>& ary)
    {
        return begin(ary) + ary.size();
    }

    // Returns whether `s` is empty (see: `std::empty`)
    template<
        std::derived_from<OpenSim::Object> T,
        std::derived_from<OpenSim::Object> C = OpenSim::Object
    >
    [[nodiscard]] bool empty(const OpenSim::Set<T, C>& s)
    {
        return size(s) <= 0;
    }

    // Returns a reference to the element of `ary` at location `pos`, with bounds checking.
    template<typename T>
    T& at(OpenSim::ArrayPtrs<T>& ary, size_t pos)
    {
        if (pos >= size(ary)) {
            throw std::out_of_range{"out of bounds access to an OpenSim::ArrayPtrs detected"};
        }

        if (T* el = ary.get(static_cast<int>(pos))) {
            return *el;
        }
        else {
            throw std::runtime_error{"attempted to access null element of ArrayPtrs"};
        }
    }

    // Returns a reference to the element of `ary` at location `pos`, with bounds checking.
    template<typename T>
    const T& at(const OpenSim::Array<T>& ary, size_t pos)
    {
        if (pos >= size(ary)) {
            throw std::out_of_range{"out of bounds access to an OpenSim::ArrayPtrs detected"};
        }
        return ary.get(static_cast<int>(pos));
    }

    // Returns a reference to the element of `s` at location `pos`, with bounds checking.
    template<
        std::derived_from<OpenSim::Object> T,
        std::derived_from<OpenSim::Object> C = OpenSim::Object
    >
    const T& at(const OpenSim::Set<T, C>& s, size_t pos)
    {
        if (pos >= size(s)) {
            throw std::out_of_range{"out of bounds access to an OpenSim::Set detected"};
        }
        return s.get(static_cast<int>(pos));
    }

    // Returns a reference to the element of `s` at location `pos`, with bounds checking.
    template<
        std::derived_from<OpenSim::Object> T,
        std::derived_from<OpenSim::Object> C = OpenSim::Object
    >
    T& at(OpenSim::Set<T, C>& s, size_t pos)
    {
        if (pos >= size(s)) {
            throw std::out_of_range{"out of bounds access to an OpenSim::Set detected"};
        }
        s.get(static_cast<int>(pos));  // force an OpenSim-based null check
        return s[static_cast<int>(pos)];
    }

    // Returns a reference to the element of `p` at location `pos`, with bounds checking.
    template<typename T>
    const T& at(const OpenSim::Property<T>& p, size_t pos)
    {
        return p[static_cast<int>(pos)];  // the implementation is already bounds-checked
    }

    template<
        std::derived_from<OpenSim::Object> T,
        std::derived_from<OpenSim::Object> C = OpenSim::Object
    >
    bool erase_at(OpenSim::Set<T, C>& s, size_t i)
    {
        return s.remove(static_cast<int>(i));
    }

    // Returns whether all elements in `v` are not equal to any other element in `v`.
    template<typename T>
    requires (std::strict_weak_order<std::ranges::less, T, T> and std::equality_comparable<T>)
    bool is_all_elements_unique(const OpenSim::Array<T>& v)
    {
        std::vector<std::reference_wrapper<const T>> buffer;
        buffer.reserve(v.size());
        std::ranges::copy(begin(v), end(v), std::back_inserter(buffer));
        std::ranges::sort(buffer, std::less<T>{});
        return std::ranges::adjacent_find(buffer, std::ranges::equal_to{}, [](const auto& wrapper) { return wrapper.get(); }) == buffer.end();
    }

    // returns true if the first argument has a lexicographically lower class name
    bool is_concrete_class_name_lexicographically_lower_than(
        const OpenSim::Component&,
        const OpenSim::Component&
    );

    // returns true if the first argument points to a component that has a lexicographically lower class
    // name than the component pointed to by second argument
    //
    // (it's a helper method that's handy for use with pointers, unique_ptr, shared_ptr, etc.)
    template<osc::DereferencesTo<const OpenSim::Component&> ComponentPtrLike>
    bool is_concrete_class_name_lexicographically_lower_than(
        const ComponentPtrLike& a,
        const ComponentPtrLike& b)
    {
        return IsConcreteClassNameLexicographicallyLowerThan(*a, *b);
    }

    bool is_name_lexicographically_lower_than(
        const OpenSim::Component&,
        const OpenSim::Component&
    );

    template<osc::DereferencesTo<const OpenSim::Component&> Ptr>
    bool is_name_lexicographically_lower_than(
        const Ptr& a,
        const Ptr& b)
    {
        return IsNameLexographicallyLowerThan(*a, *b);
    }

    template<osc::DereferencesTo<const OpenSim::Component&> Ptr>
    bool is_name_lexicographically_greater_than(
        const Ptr& a,
        const Ptr& b)
    {
        return !IsNameLexographicallyLowerThan<Ptr>(a, b);
    }

    // returns a mutable pointer to the owner (if it exists)
    OpenSim::Component* upd_owner(OpenSim::Component& root, const OpenSim::Component&);
    OpenSim::Component& upd_owner_or_throw(OpenSim::Component& root, const OpenSim::Component&);

    template<std::derived_from<OpenSim::Component> T>
    T& upd_owner_or_throw(OpenSim::Component& root, const OpenSim::Component& c)
    {
        return dynamic_cast<T&>(upd_owner_or_throw(root, c));
    }

    template<std::derived_from<OpenSim::Component> T>
    T* upd_owner(OpenSim::Component& root, const OpenSim::Component& c)
    {
        return dynamic_cast<T*>(upd_owner(root, c));
    }

    // returns a pointer to the owner (if it exists)
    const OpenSim::Component& get_owner_or_throw(const OpenSim::AbstractOutput&);
    const OpenSim::Component& get_owner_or_throw(const OpenSim::Component&);
    const OpenSim::Component& get_owner_or(const OpenSim::Component&, const OpenSim::Component& fallback);
    const OpenSim::Component* get_owner(const OpenSim::Component&);

    template<std::derived_from<OpenSim::Component> T>
    const T* get_owner(const OpenSim::Component& c)
    {
        return dynamic_cast<const T*>(get_owner(c));
    }

    template<std::derived_from<OpenSim::Component> T>
    bool owner_is(const OpenSim::Component& c)
    {
        return get_owner<T>(c) != nullptr;
    }

    std::optional<std::string> try_get_owner_name(const OpenSim::Component&);

    // returns the distance between the given `Component` and the component that is at the root of the component tree
    size_t distance_from_root(const OpenSim::Component&);

    // returns a reference to a global instance of a path that points to the root of a model (i.e. "/")
    OpenSim::ComponentPath get_root_component_path();

    // returns `true` if the given `ComponentPath` is an empty path
    bool is_empty(const OpenSim::ComponentPath&);

    // clears the given component path
    void clear(OpenSim::ComponentPath&);

    // returns all components between the root (element 0) and the given component (element n-1) inclusive
    std::vector<const OpenSim::Component*> get_path_elements(const OpenSim::Component&);

    // calls the given function with each subcomponent of the given component
    void for_each_component(const OpenSim::Component&, const std::function<void(const OpenSim::Component&)>&);

    // calls the given function with the provided component and each of its subcomponents
    void for_each_component_inclusive(const OpenSim::Component&, const std::function<void(const OpenSim::Component&)>&);

    // returns the number of children (recursive) of type T under the given component
    template<std::derived_from<OpenSim::Component> T>
    size_t get_num_children(const OpenSim::Component& c)
    {
        size_t i = 0;
        for_each_component(c, [&i](const OpenSim::Component& c)
        {
            if (dynamic_cast<const T*>(&c)) {
                ++i;
            }
        });
        return i;
    }

    // returns the number of direct children that the component owns
    size_t get_num_children(const OpenSim::Component&);

    // returns `true` if `c == parent` or `c` is a descendant of `parent`
    bool is_inclusive_child_of(
        const OpenSim::Component* parent,
        const OpenSim::Component* c
    );

    // returns the first parent in `parents` that appears to be an inclusive parent of `c`
    //
    // returns `nullptr` if no element in `parents` is an inclusive parent of `c`
    const OpenSim::Component* is_inclusive_child_of(
        std::span<const OpenSim::Component*> parents,
        const OpenSim::Component* c
    );

    // returns the first ancestor of `c` for which the given predicate returns `true`
    const OpenSim::Component* find_first_ancestor_inclusive(
        const OpenSim::Component*,
        bool(*pred)(const OpenSim::Component*)
    );

    // returns the first ancestor of `c` that has type `T`
    template<std::derived_from<OpenSim::Component> T>
    const T* find_ancestor_with_type(const OpenSim::Component* c)
    {
        const OpenSim::Component* rv = find_first_ancestor_inclusive(c, [](const OpenSim::Component* el)
        {
            return dynamic_cast<const T*>(el) != nullptr;
        });

        return dynamic_cast<const T*>(rv);
    }

    // returns `true` if `c` is a child of a component that derives from `T`
    template<std::derived_from<OpenSim::Component> T>
    bool is_child_of_a(const OpenSim::Component& c)
    {
        return find_ancestor_with_type<T>(&c) != nullptr;
    }

    // returns the first descendant (including `component`) that satisfies `predicate(descendant);`
    const OpenSim::Component* find_first_descendent_inclusive(
        const OpenSim::Component& component,
        bool(*predicate)(const OpenSim::Component&)
    );

    // returns the first descendant of `component` that satisfies `predicate(descendant)`
    const OpenSim::Component* find_first_descendent(
        const OpenSim::Component& component,
        bool(*predicate)(const OpenSim::Component&)
    );

    // returns the first descendant of `component` that satisfies `predicate(descendant)`
    OpenSim::Component* find_first_descendent_mut(
        OpenSim::Component& component,
        bool(*predicate)(const OpenSim::Component&)
    );

    // returns the first direct descendant of `component` that has type `T`, or
    // `nullptr` if no such descendant exists
    template<std::derived_from<OpenSim::Component> T>
    const T* find_first_descendent_of_type(const OpenSim::Component& c)
    {
        const OpenSim::Component* rv = FindFirstDescendent(c, [](const OpenSim::Component& el)
        {
            return dynamic_cast<const T*>(&el) != nullptr;
        });
        return dynamic_cast<const T*>(rv);
    }

    // returns the first direct descendant of `component` that has type `T`, or
    // `nullptr` if no such descendant exists
    template<std::derived_from<OpenSim::Component> T>
    T* find_first_descendent_of_type_mut(OpenSim::Component& c)
    {
        OpenSim::Component* rv = FindFirstDescendentMut(c, [](const OpenSim::Component& el)
        {
            return dynamic_cast<const T*>(&el) != nullptr;
        });
        return dynamic_cast<T*>(rv);
    }

    // returns a vector containing pointers to all user-editable coordinates in the model
    std::vector<const OpenSim::Coordinate*> get_coordinates_in_model(const OpenSim::Model&);

    // fills the given vector with all user-editable coordinates in the model
    void get_coordinates_in_model(
        const OpenSim::Model&,
        std::vector<const OpenSim::Coordinate*>&
    );

    // returns a vector containing mutable pointers to all default-locked coordinates in the model
    std::vector<OpenSim::Coordinate*> upd_default_locked_coordinates_in_model(OpenSim::Model&);

    // returns the user-facing display value (i.e. degrees) for a coordinate
    float convert_coord_value_to_display_value(const OpenSim::Coordinate&, double v);

    // returns the storage-facing value (i.e. radians) for a coordinate
    double convert_coord_display_value_to_storage_value(const OpenSim::Coordinate&, float v);

    // returns a user-facing string that describes a coordinate's units
    osc::CStringView get_coord_display_value_units_string(const OpenSim::Coordinate&);

    // returns the names of a component's sockets
    std::vector<std::string> get_socket_names(const OpenSim::Component&);

    // returns all sockets that are directly attached to the given component
    std::vector<const OpenSim::AbstractSocket*> get_all_sockets(const OpenSim::Component&);

    // returns all (mutable) sockets that are directly attached to the given component
    std::vector<OpenSim::AbstractSocket*> upd_all_sockets(OpenSim::Component&);

    // writes the given component's (recursive) topology graph to the output stream as a
    // dotviz `digraph`
    void write_component_topology_graph_as_dot_viz(
        const OpenSim::Component&,
        std::ostream&
    );

    // writes the given model's multi-body system (i.e. kinematic chain) to the output stream
    // as a dotviz `digraph`
    void write_model_multibody_system_graph_as_dot_viz(
        const OpenSim::Model&,
        std::ostream&
    );

    // returns a pointer if the given path resolves a component relative to root
    const OpenSim::Component* find_component(const OpenSim::Component& root, const OpenSim::ComponentPath&);
    const OpenSim::Component* find_component(const OpenSim::Model&, const std::string& abs_path);
    const OpenSim::Component* find_component(const OpenSim::Model&, const osc::StringName& abs_path);

    // return non-nullptr if the given path resolves a component of type T relative to root
    template<std::derived_from<OpenSim::Component> T>
    const T* find_component(const OpenSim::Component& root, const OpenSim::ComponentPath& cp)
    {
        return dynamic_cast<const T*>(find_component(root, cp));
    }

    template<std::derived_from<OpenSim::Component> T>
    const T* find_component(const OpenSim::Model& root, const std::string& cp)
    {
        return dynamic_cast<const T*>(find_component(root, cp));
    }

    template<std::derived_from<OpenSim::Component> T>
    const T* find_component(const OpenSim::Model& root, const osc::StringName& cp)
    {
        return dynamic_cast<const T*>(find_component(root, cp));
    }

    // returns a mutable pointer if the given path resolves a component relative to root
    OpenSim::Component* find_component_mut(
        OpenSim::Component& root,
        const OpenSim::ComponentPath&
    );

    // returns non-nullptr if the given path resolves a component of type T relative to root
    template<std::derived_from<OpenSim::Component> T>
    T* find_component_mut(
        OpenSim::Component& root,
        const OpenSim::ComponentPath& cp)
    {
        return dynamic_cast<T*>(find_component_mut(root, cp));
    }

    // returns true if the path resolves to a component within `root`
    bool contains_component(
        const OpenSim::Component& root,
        const OpenSim::ComponentPath&
    );

    // returns non-nullptr if a socket with the given name is found within the given component
    const OpenSim::AbstractSocket* find_socket(
        const OpenSim::Component&,
        const std::string& socketName
    );

    // returns non-nullptr if a socket with the given name is found within the given component
    OpenSim::AbstractSocket* find_socket_mut(
        OpenSim::Component&,
        const std::string& socketName
    );

    // returns `true` if the socket is connected to the component
    bool is_connected_to(
        const OpenSim::AbstractSocket&,
        const OpenSim::Component&
    );

    // returns true if the socket is able to connect to the component
    bool is_able_to_connect_to(
        const OpenSim::AbstractSocket&,
        const OpenSim::Component&
    );

    // recursively traverses all components within `root` and reassigns any sockets
    // pointing to `from` to instead point to `to`
    //
    // note: must be called on a component that already has finalized connections
    void recursively_reassign_all_sockets(
        OpenSim::Component& root,
        const OpenSim::Component& from,
        const OpenSim::Component& to
    );

    class ComponentConnectionView final {
    public:
        explicit ComponentConnectionView(
            const OpenSim::Component& source,
            const OpenSim::Component& target,
            std::string socketName) :

            source_{&source},
            target_{&target},
            socket_name_{std::move(socketName)}
        {}

        friend bool operator==(const ComponentConnectionView&, const ComponentConnectionView&) = default;

        const OpenSim::Component& source() const { return *source_; }
        const OpenSim::Component& target() const { return *target_; }
        osc::CStringView socket_name() const { return socket_name_; }
    private:
        const OpenSim::Component* source_;
        const OpenSim::Component* target_;
        std::string socket_name_;
    };
    std::ostream& operator<<(std::ostream&, const ComponentConnectionView&);

    // Returns a generator that yields `ComponentConnectionView` for each socket of each component
    // in `root` that points to `c`.
    osc::cpp23::generator<ComponentConnectionView> for_each_inbound_connection(
        const OpenSim::Component* root,
        const OpenSim::Component* c,
        std::function<bool(const OpenSim::Component&)> filter = [](const OpenSim::Component&){ return true; }
    );

    // returns a pointer to the property if the component has a property with the given name
    OpenSim::AbstractProperty* find_property_mut(
        OpenSim::Component&,
        const std::string&
    );

    // returns a pointer to the property if the component has a simple property with the given name and type
    template<typename T>
    OpenSim::SimpleProperty<T>* find_simple_property_mut(
        OpenSim::Component& c,
        const std::string& name)
    {
        return dynamic_cast<OpenSim::SimpleProperty<T>*>(find_property_mut(c, name));
    }

    // returns non-nullptr if an `AbstractOutput` with the given name is attached to the given component
    const OpenSim::AbstractOutput* find_output(
        const OpenSim::Component&,
        const std::string& output_name
    );

    // returns non-nullptr if an `AbstractOutput` with the given name is attached to a component located at the given path relative to the root
    const OpenSim::AbstractOutput* find_output(
        const OpenSim::Component& root,
        const OpenSim::ComponentPath&,
        const std::string& output_name
    );

    // returns true if the given model has an input file name (not empty, or "Unassigned")
    bool has_input_file_name(const OpenSim::Model&);

    // returns a non-empty path if the given model has an input file name that exists on the user's filesystem
    //
    // otherwise, returns an empty path
    std::optional<std::filesystem::path> try_find_input_file(const OpenSim::Model&);

    // returns the recommended name of the provided model file, e.g. for suggesting a name for users
    std::string recommended_document_name(const OpenSim::Model&);

    // returns the absolute path to the given mesh component, if found (otherwise, std::nullptr)
    std::optional<std::filesystem::path> find_geometry_file_abs_path(
        const OpenSim::Model&,
        const OpenSim::Mesh&
    );

    // returns the filename part of the `mesh_file` property (e.g. `C:\Users\adam\mesh.obj` returns `mesh.obj`)
    std::string get_mesh_file_name(const OpenSim::Mesh&);

    // returns `true` if the component should be shown in the UI
    //
    // this uses heuristics to determine whether the component is something the UI should be
    // "revealed" to the user
    bool should_show_in_ui(const OpenSim::Component&);

    // *tries* to delete the supplied component from the model
    //
    // returns `true` if the implementation was able to delete the component; otherwise, `false`
    bool try_delete_component_from_model(OpenSim::Model&, OpenSim::Component&);

    // copy common joint properties from a `src` to `dest`
    //
    // e.g. names, coordinate names, etc.
    void copy_common_joint_properties(const OpenSim::Joint& src, OpenSim::Joint& dest);

    // de-activates all wrap objects in the given model
    //
    // returns `true` if any modification was made to the model
    bool deactivate_all_wrap_objects_in(OpenSim::Model&);

    // activates all wrap objects in the given model
    //
    // returns `true` if any modification was made to the model
    bool activate_all_wrap_objects_in(OpenSim::Model&);

    // returns pointers to all wrap objects that are referenced by the given `GeometryPath`
    std::vector<const OpenSim::WrapObject*> get_all_wrap_objects_referenced_by(const OpenSim::GeometryPath&);

    // Returns `true` if `path` has a supported model file extension.
    bool has_model_file_extension(const std::filesystem::path& path);

    // returns a pointer to a not-yet-initialized model, loaded via an osim file at the given path
    std::unique_ptr<OpenSim::Model> load_model(const std::filesystem::path&);

    // fully initialize an OpenSim model (clear connections, finalize properties, remake SimTK::System)
    void initialize_model(OpenSim::Model&);

    // Tries to equilibrate the muscles in the given model for the given state, or logs a warning
    // message if the muscles cannot be equilibriated.
    //
    // This should be used in the UI in cases where the user may load or edit a model that contains
    // invalid/incorrect muscles. Some OpenSim models can have this problem, and it shouldn't be
    // treated as a fatal error (opensim-creator/#1070).
    void try_equilibrate_muscles_or_log_warning(OpenSim::Model&, SimTK::State&);

    // fully initalize an OpenSim model's working state
    SimTK::State& initialize_state(OpenSim::Model&);

    // calls `model.finalizeFromProperties()`
    //
    // (mostly here to match the style of osc's initialization methods)
    void finalize_from_properties(OpenSim::Model&);

    // finalize any socket connections in the model
    //
    // care:
    //
    // - it will _first_ finalize any properties in the model
    // - then it will finalize each socket recursively
    // - finalizing a socket causes the socket's pointer to write an updated
    //   component path to the socket's path property (for later serialization)
    void finalize_connections(OpenSim::Model&);

    // returns optional{index} if joint is found in parent jointset (otherwise: std::nullopt)
    std::optional<size_t> find_joint_in_parent_joint_set(const OpenSim::Joint&);

    // returns user-visible (basic) name of geometry, or underlying file name
    std::string get_display_name(const OpenSim::Geometry&);

    // returns a user-visible string for a coordinate's motion type
    osc::CStringView get_motion_type_display_name(const OpenSim::Coordinate&);

    // returns a pointer to the component's appearance property, or `nullptr` if it doesn't have one
    const OpenSim::Appearance* try_get_appearance(const OpenSim::Component&);
    OpenSim::Appearance* try_upd_appearance(OpenSim::Component&);

    // tries to set the given component's appearance property's visibility field to the given bool
    //
    // returns `false` if the component doesn't have an appearance property, `true` if it does (and
    // the value was set)
    bool try_set_appearance_property_is_visible_to(OpenSim::Component&, bool);

    // returns the color part of the `OpenSim::Appearance` as an `osc::Color`
    osc::Color to_color(const OpenSim::Appearance&);

    osc::Color get_suggested_bone_color();  // best guess, based on shaders etc.

    // returns `true` if the given model's display properties asks to show frames
    bool is_showing_frames(const OpenSim::Model&);

    // toggles the model's "show frames" display property and returns the new value
    bool toggle_showing_frames(OpenSim::Model&);

    // returns `true` if the given model's display properties ask to show markers
    bool is_showing_markers(const OpenSim::Model&);

    // toggles the model's "show markers" display property and returns the new value
    bool toggle_showing_markers(OpenSim::Model&);

    // returns `true` if the given model's display properties asks to show wrap geometry
    bool is_showing_wrap_geometry(const OpenSim::Model&);

    // toggles the model's "show wrap geometry" display property and returns the new value
    bool toggle_showing_wrap_geometry(OpenSim::Model&);

    // returns `true` if the given model's display properties asks to show contact geometry
    bool is_showing_contact_geometry(const OpenSim::Model&);

    // returns `true` if the given model's display properties asks to show forces
    bool is_showing_forces(const OpenSim::Model&);

    // toggles the model's "show contact geometry" display property and returns the new value
    bool toggle_showing_contact_geometry(OpenSim::Model&);

    // toggles the model's "show forces" display property and returns the new value
    bool toggle_showing_forces(OpenSim::Model&);

    // returns/assigns the absolute path to a component within its hierarchy (e.g. /jointset/joint/somejoint)
    //
    // (custom OSC version that may be faster than OpenSim::Component::getAbsolutePathString)
    void get_absolute_path_string(const OpenSim::Component&, std::string&);
    std::string get_absolute_path_string(const OpenSim::Component&);
    osc::StringName get_absolute_path_string_name(const OpenSim::Component&);

    // returns the absolute path to a component within its hierarchy (e.g. /jointset/joint/somejoint)
    //
    // (custom OSC version that may be faster than OpenSim::Component::getAbsolutePath)
    OpenSim::ComponentPath get_absolute_path(const OpenSim::Component&);

    // if non-nullptr, returns/assigns the absolute path to the component within its hierarchy (e.g. /jointset/joint/somejoint)
    //
    // (custom OSC version that may be faster than OpenSim::Component::getAbsolutePath)
    OpenSim::ComponentPath get_absolute_path_or_empty(const OpenSim::Component*);

    // muscle lines of action
    //
    // helper functions for computing the "line of action" of a muscle. These algorithms were
    // adapted from: https://github.com/modenaxe/MuscleForceDirection/
    //
    // the reason they return `optional` is to handle edge-cases like the path containing an
    // insufficient number of points (shouldn't happen, but you never know)
    struct LinesOfAction final {
        osc::Ray origin;
        osc::Ray insertion;
    };
    std::optional<LinesOfAction> get_effective_lines_of_action_in_ground(const OpenSim::Muscle&, const SimTK::State&);
    std::optional<LinesOfAction> get_anatomical_lines_of_action_in_ground(const OpenSim::Muscle&, const SimTK::State&);

    // contact forces
    //
    // helper functions for pulling contact forces out of the model (e.g. for rendering)
    struct ForcePoint final {
        osc::Vector3 force;
        osc::Vector3 point;
    };
    std::optional<ForcePoint> try_get_contact_force_in_ground(
        const OpenSim::Model&,
        const SimTK::State&,
        const OpenSim::HuntCrossleyForce&
    );

    // force vectors
    //
    // helper functions for pulling force vectors out of components in the model
    const OpenSim::PhysicalFrame& get_frame_using_external_force_lookup_heuristic(
        const OpenSim::Model&,
        const std::string& body_name_or_path
    );

    // point info
    //
    // extract point-like information from generic OpenSim components
    struct PointInfo final {
        PointInfo(
            osc::Vector3 location_,
            OpenSim::ComponentPath frameAbsPath_) :

            location{location_},
            frame_abs_path{std::move(frameAbsPath_)}
        {}

        osc::Vector3 location;
        OpenSim::ComponentPath frame_abs_path;
    };
    bool can_extract_point_info_from(const OpenSim::Component&, const SimTK::State&);
    std::optional<PointInfo> try_extract_point_info(const OpenSim::Component&, const SimTK::State&);

    // adds a component to an appropriate location in the model (e.g. joint-set for a joint) and
    // returns a reference to the placed component
    OpenSim::Component& add_component_to_appropriate_set(OpenSim::Model&, std::unique_ptr<OpenSim::Component>);

    // adds a model component to the component set of a model and returns a reference to the component
    OpenSim::ModelComponent& add_model_component(OpenSim::Model&, std::unique_ptr<OpenSim::ModelComponent>&&);

    // adds a specific (T) model component to the component set of the model and returns a reference to the component
    template<std::derived_from<OpenSim::ModelComponent> T>
    T& add_model_component(OpenSim::Model& model, std::unique_ptr<T> p)
    {
        return static_cast<T&>(add_model_component(model, static_cast<std::unique_ptr<OpenSim::ModelComponent>&&>(std::move(p))));
    }

    // constructs a specific (T) model component in the component set of the model and returns a reference to the component
    template<std::derived_from<OpenSim::ModelComponent> T, typename... Args>
    requires std::constructible_from<T, Args&&...>
    T& add_model_component(OpenSim::Model& model, Args&&... args)
    {
        return add_model_component(model, std::make_unique<T>(std::forward<Args>(args)...));
    }

    // adds a new component to the component set of the component and returns a reference to the new component
    OpenSim::Component& add_component(OpenSim::Component&, std::unique_ptr<OpenSim::Component>&&);

    template<std::derived_from<OpenSim::Component> T>
    T& add_component(OpenSim::Component& c, std::unique_ptr<T> p)
    {
        return static_cast<T&>(add_component(c, static_cast<std::unique_ptr<OpenSim::Component>&&>(std::move(p))));
    }

    template<std::derived_from<OpenSim::Component> T>
    T& add_component(OpenSim::Component& host)
    {
        return add_component(host, std::make_unique<T>());
    }

    template<std::derived_from<OpenSim::Component> T, typename... Args>
    requires std::constructible_from<T, Args&&...>
    T& add_component(OpenSim::Component& host, Args&&...args)
    {
        return add_component(host, std::make_unique<T>(std::forward<Args>(args)...));
    }

    OpenSim::Body& add_body(OpenSim::Model&, std::unique_ptr<OpenSim::Body>);

    template<typename... Args>
    requires std::constructible_from<OpenSim::Body, Args&&...>
    OpenSim::Body& add_body(OpenSim::Model& model, Args&&... args)
    {
        auto p = std::make_unique<OpenSim::Body>(std::forward<Args>(args)...);
        return add_body(model, std::move(p));
    }

    OpenSim::Joint& add_joint(OpenSim::Model&, std::unique_ptr<OpenSim::Joint>);

    template<std::derived_from<OpenSim::Joint> T, typename... Args>
    requires std::constructible_from<T, Args&&...>
    T& add_joint(OpenSim::Model& model, Args&&... args)
    {
        auto p = std::make_unique<T>(std::forward<Args>(args)...);
        return static_cast<T&>(add_joint(model, std::move(p)));
    }

    OpenSim::Constraint& add_constraint(OpenSim::Model&, std::unique_ptr<OpenSim::Constraint>);

    template<std::derived_from<OpenSim::Constraint> T, typename... Args>
    requires std::constructible_from<T, Args&&...>
    T& add_constraint(OpenSim::Model& model, Args&&... args)
    {
        auto p = std::make_unique<T>(std::forward<Args>(args)...);
        return static_cast<T&>(add_constraint(model, std::move(p)));
    }

    OpenSim::Marker& add_marker(OpenSim::Model&, std::unique_ptr<OpenSim::Marker>);

    template<typename... Args>
    requires std::constructible_from<OpenSim::Marker, Args&&...>
    OpenSim::Marker& add_marker(OpenSim::Model& model, Args&&... args)
    {
        auto p = std::make_unique<OpenSim::Marker>(std::forward<Args>(args)...);
        return add_marker(model, std::move(p));
    }

    OpenSim::Geometry& attach_geometry(OpenSim::Frame&, std::unique_ptr<OpenSim::Geometry>);

    template<std::derived_from<OpenSim::Geometry> T, typename... Args>
    requires std::constructible_from<T, Args&&...>
    T& attach_geometry(OpenSim::Frame& frame, Args&&... args)
    {
        auto p = std::make_unique<T>(std::forward<Args>(args)...);
        return static_cast<T&>(attach_geometry(frame, std::move(p)));
    }

    // Tries to overwrite `old_geometry` in the given `model` with `new_geometry`.
    //
    // This is useful when transforming geometry (e.g. TPS warping) and overwriting it
    // in a model.
    void overwrite_geometry(
        OpenSim::Model&,
        OpenSim::Geometry& old_geometry,
        std::unique_ptr<OpenSim::Geometry> new_geometry
    );

    OpenSim::PhysicalOffsetFrame& add_frame(OpenSim::Joint&, std::unique_ptr<OpenSim::PhysicalOffsetFrame>);

    OpenSim::WrapObject& add_wrap_object(OpenSim::PhysicalFrame&, std::unique_ptr<OpenSim::WrapObject>);

    template<std::derived_from<OpenSim::WrapObject> T, typename... Args>
    requires std::constructible_from<T, Args&&...>
    T& add_wrap_object(OpenSim::PhysicalFrame& physFrame, Args&&... args)
    {
        return static_cast<T&>(add_wrap_object(physFrame, std::make_unique<T>(std::forward<Args>(args)...)));
    }

    template<ClonesToRawPointer T>
    std::unique_ptr<T> clone(const T& obj)
    {
        return std::unique_ptr<T>(obj.clone());
    }

    template<std::derived_from<OpenSim::Component> T>
    void append(OpenSim::ObjectProperty<T>& prop, const T& c)
    {
        prop.adoptAndAppendValue(clone(c).release());
    }

    template<
        std::derived_from<OpenSim::Object> T,
        std::derived_from<OpenSim::Object> C = OpenSim::Object
    >
    std::optional<size_t> index_of(const OpenSim::Set<T, C>& set, const T& el)
    {
        for (size_t i = 0; i < size(set); ++i) {
            if (&at(set, i) == &el) {
                return i;
            }
        }
        return std::nullopt;
    }

    template<
        std::derived_from<OpenSim::Object> T,
        std::derived_from<T> U,
        std::derived_from<OpenSim::Object> C = OpenSim::Object
    >
    void append(OpenSim::Set<T, C>& set, std::unique_ptr<U> el)
    {
        set.adoptAndAppend(el.release());
    }

    template<
        std::derived_from<OpenSim::Object> T,
        std::derived_from<T> U,
        std::derived_from<OpenSim::Object> C = OpenSim::Object
    >
    U& assign(OpenSim::Set<T, C>& set, size_t index, std::unique_ptr<U> el)
    {
        if (index >= size(set)) {
            throw std::out_of_range{"out of bounds access to an OpenSim::Set detected"};
        }

        U& rv = *el;
        set.set(static_cast<int>(index), el.release());
        return rv;
    }

    template<
        std::derived_from<OpenSim::Object> T,
        std::derived_from<T> U,
        std::derived_from<OpenSim::Object> C = OpenSim::Object
    >
    U& assign(OpenSim::Set<T, C>& set, T& oldElement, std::unique_ptr<U> newElement)
    {
        auto idx = index_of(set, oldElement);
        if (not idx) {
            throw std::runtime_error{"cannot find the requested element in the set"};
        }
        return assign(set, *idx, std::move(newElement));
    }

    template<
        std::derived_from<OpenSim::Object> T,
        std::derived_from<T> U,
        std::derived_from<OpenSim::Object> C = OpenSim::Object
    >
    U& assign(OpenSim::Set<T, C>& set, size_t index, const U& el)
    {
        return assign(set, index, clone(el));
    }

    // Tries to delete an item from an `OpenSim::Set`.
    //
    // Returns `true` if the item was found and deleted; otherwise, returns `false`.
    template<
        std::derived_from<OpenSim::Object> T,
        std::derived_from<T> U,
        std::derived_from<OpenSim::Object> C
    >
    bool try_delete_item_from_set(OpenSim::Set<T, C>& set, const U* item)
    {
        for (size_t i = 0; i < size(set); ++i) {
            if (&at(set, i) == item) {
                return erase_at(set, i);
            }
        }
        return false;
    }

    // tries to get the "parent" frame of the given component (if available)
    //
    // e.g. in OpenSim, this is usually acquired with `getParentFrame()`
    //      but that API isn't exposed generically via virtual methods
    const OpenSim::PhysicalFrame* try_get_parent_to_ground_frame(const OpenSim::Component&);

    // tries to get the "parent" transform of the given component (if available)
    //
    // e.g. in OpenSim, this is usually acquired with `getParentFrame().getTransformInGround(State const&)`
    //      but that API isn't exposed generically via virtual methods
    std::optional<SimTK::Transform> try_get_parent_to_ground_transform(const OpenSim::Component&, const SimTK::State&);

    // tries to get the name of the "positional" property of the given component
    //
    // e.g. the positional property of an `OpenSim::Station` is "location", whereas the
    //      positional property of an `OpenSim::PhysicalOffsetFrame` is "translation"
    std::optional<std::string> try_get_positional_property_name(const OpenSim::Component&);

    // tries to get the name of the "orientational" property of the given component
    //
    // e.g. the orientational property of an `OpenSim::PhysicalOffsetFrame` is "orientation",
    //      whereas `OpenSim::Station` has no orientation, so this returns `std::nullopt`
    std::optional<std::string> try_get_orientational_property_name(const OpenSim::Component&);

    // tries to return the "parent" of the given frame, if applicable (e.g. if the frame is an
    // `OffsetFrame<T>` that has a logical parent
    const OpenSim::Frame* try_get_parent_frame(const OpenSim::Frame&);

    // packages up the various useful parts that describe how a component is spatially represented
    //
    // see also (component functions):
    //
    // - try_get_parent_to_ground_transform
    // - try_get_positional_property_name
    // - try_get_orientational_property_name
    struct ComponentSpatialRepresentation final {
        SimTK::Transform parent_to_ground;
        std::string location_vec3_property_name;
        std::optional<std::string> maybe_orientation_vec3_eulers_property_name;
    };

    // tries to get the "spatial" representation of a component
    std::optional<ComponentSpatialRepresentation> try_get_spatial_representation(
        const OpenSim::Component&,
        const SimTK::State&
    );

    // returns `true` if the given character is permitted to appear within the name
    // of an `OpenSim::Component`
    bool is_valid_open_sim_component_name_character(char);

    // returns a sanitized form of the given string that OpenSim would (probably) accept
    // as a component name
    std::string sanitize_to_open_sim_component_name(std::string_view);

    struct StorageLoadingParameters final {
        bool convert_rotational_values_to_radians = true;
        std::optional<double> resample_to_frequency = std::nullopt;
    };

    // returns an `OpenSim::Storage` with the given parameters
    //
    // (a `Model` is required because its `Coordinate`s are used to figure out which columns
    //  might be rotational)
    std::unique_ptr<OpenSim::Storage> load_storage(
        const OpenSim::Model&,
        const std::filesystem::path&,
        const StorageLoadingParameters& = {}
    );

    // Represents the result of trying to map columns in an `OpenSim::Storage` to state
    // variables in an `OpenSim::Model`.
    struct StorageIndexToModelStateVarMappingResult final {
        std::unordered_map<int, int> storage_index_to_model_state_var_index;
        std::vector<std::string> state_variables_missing_in_storage;
    };

    StorageIndexToModelStateVarMappingResult create_storage_index_to_model_statevar_mapping(
        const OpenSim::Model&,
        const OpenSim::Storage&
    );

    std::unordered_map<int, int> create_storage_index_to_model_statevar_mapping_with_warnings(
        const OpenSim::Model&,
        const OpenSim::Storage&
    );

    void update_state_variables_from_storage_row(
        OpenSim::Model&,
        SimTK::State&,
        const std::unordered_map<int, int>& column_index_to_model_state_var_index,
        const OpenSim::Storage&,
        int row
    );

    void update_state_from_storage_time(
        OpenSim::Model&,
        SimTK::State&,
        const std::unordered_map<int, int>& column_index_to_model_state_var_index,
        const OpenSim::Storage&,
        double time
    );

    std::string write_object_xml_to_string(const OpenSim::Object&);

    // Scales the masses of all bodies in `model` such that its total mass
    // becomes equal to `new_mass`, while preserving the relative distribution
    // of masses of the model.
    //
    // Note: this edits the body mass (properties), but doesn't re-initialize
    // `model` or `state`.
    void scale_model_mass_preserve_mass_distribution(
        OpenSim::Model& model,
        const SimTK::State& state,
        double new_mass
    );

    // Bakes any `StationDefinedFrame`s in `model` to legacy-compatible
    // `PhysicalOffsetFrame`s.
    void bake_station_defined_frames(OpenSim::Model&);
}
