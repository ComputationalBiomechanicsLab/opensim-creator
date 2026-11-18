#pragma once

#include <libopensimcreator/documents/mesh_importer/mi_object_finder.h>
#include <libopensimcreator/documents/mesh_importer/mi_object.h>

#include <liboscar/utilities/clone_ptr.h>
#include <liboscar/utilities/uid.h>

#include <concepts>
#include <cstddef>
#include <iterator>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <vector>

namespace osc
{
    // document support
    //
    // objects are collected into a single, potentially interconnected, graph datastructure.
    // This datastructure is what ultimately maps into an "OpenSim::Model".
    //
    // Main design considerations:
    //
    // - Must have somewhat fast associative lookup semantics, because the UI needs to
    //   traverse the graph in a value-based (rather than pointer-based) way
    //
    // - Must have value semantics, so that other code such as the undo/redo buffer can
    //   copy an entire document somewhere else in memory without having to worry about
    //   aliased mutations
    class MiDocument final : public MiObjectFinder {

        using ObjectLookup = std::map<UID, ClonePtr<MiObject>>;

        // helper class for iterating over document objects
        template<std::derived_from<MiObject> T>
        class Iterator final {
        public:
            using difference_type = ptrdiff_t;
            using value_type = T;
            using pointer = T*;
            using const_pointer = const T*;
            using reference = T&;
            using iterator_category = std::forward_iterator_tag;

            // caller-provided iterator
            using InternalIterator = std::conditional_t<
                std::is_const_v<T>,
                ObjectLookup::const_iterator,
                ObjectLookup::iterator>;

            Iterator() = default;

            Iterator(InternalIterator pos, InternalIterator end) :
                pos_{pos},
                end_{end}
            {
                // ensure iterator initially points at an object with the correct type
                while (pos_ != end_) {
                    if (dynamic_cast<const_pointer>(pos_->second.get())) {
                        break;
                    }
                    ++pos_;
                }
            }

            // conversion to a const version of the iterator
            explicit operator Iterator<const value_type>() const
            {
                return Iterator<const value_type>{pos_, end_};
            }

            // LegacyIterator

            Iterator& operator++()
            {
                while (++pos_ != end_) {
                    if (dynamic_cast<const_pointer>(pos_->second.get())) {
                        break;
                    }
                }
                return *this;
            }

            Iterator operator++(int)
            {
                Iterator copy{*this};
                ++(*this);
                return copy;
            }

            reference operator*() const { return dynamic_cast<reference>(*pos_->second); }

            // EqualityComparable

            friend bool operator==(const Iterator&, const Iterator&) = default;

            // LegacyInputIterator

            pointer operator->() const { return &dynamic_cast<reference>(*pos_->second); }

        private:
            InternalIterator pos_{};
            InternalIterator end_{};
        };

        // helper class for an iterable object with a beginning + end
        template<std::derived_from<MiObject> T>
        class Iterable final {
        public:
            using MapRef =
                std::conditional_t<std::is_const_v<T>, const ObjectLookup&, ObjectLookup&>;

            explicit Iterable(MapRef objects) :
                begin_{objects.begin(), objects.end()},
                end_{objects.end(), objects.end()}
            {}

            Iterator<T> begin() const { return begin_; }
            Iterator<T> end() const { return end_; }

        private:
            Iterator<T> begin_;
            Iterator<T> end_;
        };

    public:
        MiDocument();

        template<std::derived_from<MiObject> T = MiObject>
        T* try_upd_by_id(UID id)
        {
            return find_by_id<T>(objects_, id);
        }

        template<std::derived_from<MiObject> T = MiObject>
        const T* try_get_by_id(UID id) const
        {
            return find_by_id<T>(objects_, id);
        }

        template<std::derived_from<MiObject> T = MiObject>
        T& upd_by_id(UID id)
        {
            return find_by_id_or_throw<T>(objects_, id);
        }

        template<std::derived_from<MiObject> T = MiObject>
        const T& get_by_id(UID id) const
        {
            return find_by_id_or_throw<T>(objects_, id);
        }

        CStringView get_label_by_id(UID id) const { return get_by_id(id).get_label(); }

        Transform get_x_form_by_id(UID id) const { return get_by_id(id).get_x_form(*this); }

        Vector3 get_pos_by_id(UID id) const { return get_by_id(id).get_pos(*this); }

        template<std::derived_from<MiObject> T = MiObject>
        bool contains(UID id) const
        {
            return try_get_by_id<T>(id);
        }

        template<std::derived_from<MiObject> T = MiObject>
        bool contains(const MiObject& e) const
        {
            return contains<T>(e.get_id());
        }

        template<std::derived_from<MiObject> T = MiObject>
        Iterable<T> iter()
        {
            return Iterable<T>{objects_};
        }

        template<std::derived_from<MiObject> T = MiObject>
        Iterable<const T> iter() const
        {
            return Iterable<const T>{objects_};
        }

        MiObject& insert(std::unique_ptr<MiObject> obj)
        {
            // ensure object connects to things that already exist in the document
            for (int i = 0, len = obj->get_num_cross_references(); i < len; ++i) {
                if (!contains(obj->get_cross_reference_connectee_id(i))) {
                    std::stringstream ss;
                    ss << "cannot add '" << obj->get_label() << "' (ID = " << obj->get_id()
                       << ") to the document because it contains a cross reference (label = "
                       << obj->get_cross_reference_label(i)
                       << ") to another object that does not exist in the document";
                    throw std::runtime_error{std::move(ss).str()};
                }
            }

            return *objects_.emplace(obj->get_id(), std::move(obj)).first->second;
        }

        template<std::derived_from<MiObject> T, typename... Args>
        requires std::constructible_from<T, Args&&...>
        T& emplace(Args&&... args)
        {
            return static_cast<T&>(insert(std::make_unique<T>(std::forward<Args>(args)...)));
        }

        bool delete_by_id(UID id)
        {
            const MiObject* const obj = try_get_by_id(id);
            if (!obj) {
                return false;  // ID doesn't exist in the document
            }

            // collect all to-be-deleted objects into one deletion set so that the deletion
            // happens in separate phase from the "search for things to delete" phase
            std::unordered_set<UID> deletion_set;
            populate_deletion_set(*obj, deletion_set);

            for (UID deleted_id : deletion_set) {
                de_select(deleted_id);

                // move object into deletion set, rather than deleting it immediately,
                // so that code that relies on references to the to-be-deleted object
                // still works until an explicit `.garbage_collect()` call
                if (const auto it = objects_.find(deleted_id); it != objects_.end()) {
                    deleted_objects_.push_back(std::move(it->second));
                    objects_.erase(it);
                }
            }

            return !deletion_set.empty();
        }

        void garbage_collect() { deleted_objects_.clear(); }

        // selection logic

        bool has_selection() const { return !selected_object_ids_.empty(); }

        const std::unordered_set<UID>& get_selected() const { return selected_object_ids_; }

        bool is_selected(UID id) const { return selected_object_ids_.contains(id); }

        bool is_selected(const MiObject& obj) const { return is_selected(obj.get_id()); }

        void select(UID id)
        {
            const MiObject* const e = try_get_by_id(id);

            if (e && e->can_select()) {
                selected_object_ids_.insert(id);
            }
        }

        void select(const MiObject& obj) { select(obj.get_id()); }

        void select_only(const MiObject& obj)
        {
            de_select_all();
            select(obj);
        }

        void de_select(UID id) { selected_object_ids_.erase(id); }

        void de_select(const MiObject& obj) { de_select(obj.get_id()); }

        void select_all()
        {
            for (const MiObject& obj : iter()) {
                if (obj.can_select()) {
                    selected_object_ids_.insert(obj.get_id());
                }
            }
        }

        void de_select_all() { selected_object_ids_.clear(); }

        void delete_selected()
        {
            // copy deletion set to ensure iterator can't be invalidated by deletion
            std::unordered_set<UID> selected = get_selected();

            for (UID id : selected) {
                delete_by_id(id);
            }

            de_select_all();
        }

    private:
        template<std::derived_from<MiObject> T = MiObject, typename AssociativeContainer>
        static T* find_by_id(AssociativeContainer& container, UID id)
        {
            const auto it = container.find(id);

            if (it == container.end()) {
                return nullptr;
            }

            if constexpr (std::is_same_v<T, MiObject>) {
                return it->second.get();
            } else {
                return dynamic_cast<T*>(it->second.get());
            }
        }

        template<std::derived_from<MiObject> T = MiObject, typename AssociativeContainer>
        static T& find_by_id_or_throw(AssociativeContainer& container, UID id)
        {
            T* ptr = find_by_id<T>(container, id);
            if (!ptr) {
                std::stringstream msg;
                msg << "could not find an object of type " << typeid(T).name()
                    << " with ID = " << id;
                throw std::runtime_error{std::move(msg).str()};
            }

            return *ptr;
        }

        const MiObject* impl_find(UID id) const override { return find_by_id(objects_, id); }

        void populate_deletion_set(const MiObject& deletion_target, std::unordered_set<UID>& out)
        {
            const UID deleted_id = deletion_target.get_id();

            // add the deletion target to the deletion set (if applicable)
            if (deletion_target.can_delete()) {
                if (!out.emplace(deleted_id).second) {
                    throw std::runtime_error{"cannot populate deletion set - cycle detected"};
                }
            }

            // iterate over everything else in the document and look for things
            // that cross-reference the to-be-deleted object - those things should
            // also be deleted
            for (const MiObject& obj : iter()) {
                if (obj.is_cross_referencing(deleted_id)) {
                    populate_deletion_set(obj, out);
                }
            }
        }

        ObjectLookup objects_;
        std::unordered_set<UID> selected_object_ids_;
        std::vector<ClonePtr<MiObject>> deleted_objects_;
    };
}
