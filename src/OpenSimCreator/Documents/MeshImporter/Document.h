#pragma once

#include <OpenSimCreator/Documents/MeshImporter/IObjectFinder.h>
#include <OpenSimCreator/Documents/MeshImporter/MIObject.h>

#include <oscar/Utils/ClonePtr.h>
#include <oscar/Utils/UID.h>

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

namespace osc::mi
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
    class Document final : public IObjectFinder {

        using ObjectLookup = std::map<UID, ClonePtr<MIObject>>;

        // helper class for iterating over document objects
        template<std::derived_from<MIObject> T>
        class Iterator final {
        public:
            using difference_type = ptrdiff_t;
            using value_type = T;
            using pointer = T*;
            using const_pointer = T const*;
            using reference = T const&;
            using iterator_category = std::forward_iterator_tag;

            // caller-provided iterator
            using InternalIterator = std::conditional_t<
                std::is_const_v<T>,
                ObjectLookup::const_iterator,
                ObjectLookup::iterator
            >;

            Iterator() = default;

            Iterator(InternalIterator pos_, InternalIterator end_) :
                m_Pos{pos_},
                m_End{end_}
            {
                // ensure iterator initially points at an object with the correct type
                while (m_Pos != m_End)
                {
                    if (dynamic_cast<const_pointer>(m_Pos->second.get()))
                    {
                        break;
                    }
                    ++m_Pos;
                }
            }

            // conversion to a const version of the iterator
            explicit operator Iterator<value_type const>() const
            {
                return Iterator<value_type const>{m_Pos, m_End};
            }

            // LegacyIterator

            Iterator& operator++()
            {
                while (++m_Pos != m_End)
                {
                    if (dynamic_cast<const_pointer>(m_Pos->second.get()))
                    {
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

            reference operator*() const
            {
                return dynamic_cast<reference>(*m_Pos->second);
            }

            // EqualityComparable

            friend bool operator==(const Iterator&, const Iterator&) = default;

            // LegacyInputIterator

            pointer operator->() const
            {

                return &dynamic_cast<reference>(*m_Pos->second);
            }

        private:
            InternalIterator m_Pos{};
            InternalIterator m_End{};
        };

        // helper class for an iterable object with a beginning + end
        template<std::derived_from<MIObject> T>
        class Iterable final {
        public:
            using MapRef = std::conditional_t<std::is_const_v<T>, const ObjectLookup&, ObjectLookup&>;

            explicit Iterable(MapRef objects) :
                m_Begin{objects.begin(), objects.end()},
                m_End{objects.end(), objects.end()}
            {
            }

            Iterator<T> begin() const { return m_Begin; }
            Iterator<T> end() const { return m_End; }

        private:
            Iterator<T> m_Begin;
            Iterator<T> m_End;
        };

    public:

        Document();

        template<std::derived_from<MIObject> T = MIObject>
        T* tryUpdByID(UID id)
        {
            return findByID<T>(m_Objects, id);
        }

        template<std::derived_from<MIObject> T = MIObject>
        T const* tryGetByID(UID id) const
        {
            return findByID<T>(m_Objects, id);
        }

        template<std::derived_from<MIObject> T = MIObject>
        T& updByID(UID id)
        {
            return findByIDOrThrow<T>(m_Objects, id);
        }

        template<std::derived_from<MIObject> T = MIObject>
        T const& getByID(UID id) const
        {
            return findByIDOrThrow<T>(m_Objects, id);
        }

        CStringView getLabelByID(UID id) const
        {
            return getByID(id).getLabel();
        }

        Transform getXFormByID(UID id) const
        {
            return getByID(id).getXForm(*this);
        }

        Vec3 getPosByID(UID id) const
        {
            return getByID(id).getPos(*this);
        }

        template<std::derived_from<MIObject> T = MIObject>
        bool contains(UID id) const
        {
            return tryGetByID<T>(id);
        }

        template<std::derived_from<MIObject> T = MIObject>
        bool contains(const MIObject& e) const
        {
            return contains<T>(e.getID());
        }

        template<std::derived_from<MIObject> T = MIObject>
        Iterable<T> iter()
        {
            return Iterable<T>{m_Objects};
        }

        template<std::derived_from<MIObject> T = MIObject>
        Iterable<T const> iter() const
        {
            return Iterable<T const>{m_Objects};
        }

        MIObject& insert(std::unique_ptr<MIObject> obj)
        {
            // ensure object connects to things that already exist in the document
            for (int i = 0, len = obj->getNumCrossReferences(); i < len; ++i)
            {
                if (!contains(obj->getCrossReferenceConnecteeID(i)))
                {
                    std::stringstream ss;
                    ss << "cannot add '" << obj->getLabel() << "' (ID = " << obj->getID() << ") to the document because it contains a cross reference (label = " << obj->getCrossReferenceLabel(i) << ") to another object that does not exist in the document";
                    throw std::runtime_error{std::move(ss).str()};
                }
            }

            return *m_Objects.emplace(obj->getID(), std::move(obj)).first->second;
        }

        template<std::derived_from<MIObject> T, typename... Args>
        requires std::constructible_from<T, Args&&...>
        T& emplace(Args&&... args)
        {
            return static_cast<T&>(insert(std::make_unique<T>(std::forward<Args>(args)...)));
        }

        bool deleteByID(UID id)
        {
            MIObject const* const obj = tryGetByID(id);
            if (!obj)
            {
                return false;  // ID doesn't exist in the document
            }

            // collect all to-be-deleted objects into one deletion set so that the deletion
            // happens in separate phase from the "search for things to delete" phase
            std::unordered_set<UID> deletionSet;
            populateDeletionSet(*obj, deletionSet);

            for (UID deletedID : deletionSet)
            {
                deSelect(deletedID);

                // move object into deletion set, rather than deleting it immediately,
                // so that code that relies on references to the to-be-deleted object
                // still works until an explicit `.GarbageCollect()` call
                if (auto const it = m_Objects.find(deletedID); it != m_Objects.end())
                {
                    m_DeletedObjects.push_back(std::move(it->second));
                    m_Objects.erase(it);
                }
            }

            return !deletionSet.empty();
        }

        void garbageCollect()
        {
            m_DeletedObjects.clear();
        }


        // selection logic

        bool hasSelection() const
        {
            return !m_SelectedObjectIDs.empty();
        }

        std::unordered_set<UID> const& getSelected() const
        {
            return m_SelectedObjectIDs;
        }

        bool isSelected(UID id) const
        {
            return m_SelectedObjectIDs.contains(id);
        }

        bool isSelected(const MIObject& obj) const
        {
            return isSelected(obj.getID());
        }

        void select(UID id)
        {
            MIObject const* const e = tryGetByID(id);

            if (e && e->canSelect())
            {
                m_SelectedObjectIDs.insert(id);
            }
        }

        void select(const MIObject& obj)
        {
            select(obj.getID());
        }

        void selectOnly(const MIObject& obj)
        {
            deSelectAll();
            select(obj);
        }

        void deSelect(UID id)
        {
            m_SelectedObjectIDs.erase(id);
        }

        void deSelect(const MIObject& obj)
        {
            deSelect(obj.getID());
        }

        void selectAll()
        {
            for (const MIObject& obj : iter())
            {
                if (obj.canSelect())
                {
                    m_SelectedObjectIDs.insert(obj.getID());
                }
            }
        }

        void deSelectAll()
        {
            m_SelectedObjectIDs.clear();
        }

        void deleteSelected()
        {
            // copy deletion set to ensure iterator can't be invalidated by deletion
            std::unordered_set<UID> selected = getSelected();

            for (UID id : selected)
            {
                deleteByID(id);
            }

            deSelectAll();
        }
    private:
        template<
            std::derived_from<MIObject> T = MIObject,
            typename AssociativeContainer
        >
        static T* findByID(AssociativeContainer& container, UID id)
        {
            auto const it = container.find(id);

            if (it == container.end())
            {
                return nullptr;
            }

            if constexpr (std::is_same_v<T, MIObject>)
            {
                return it->second.get();
            }
            else
            {
                return dynamic_cast<T*>(it->second.get());
            }
        }

        template<
            std::derived_from<MIObject> T = MIObject,
            typename AssociativeContainer
        >
        static T& findByIDOrThrow(AssociativeContainer& container, UID id)
        {
            T* ptr = findByID<T>(container, id);
            if (!ptr)
            {
                std::stringstream msg;
                msg << "could not find an object of type " << typeid(T).name() << " with ID = " << id;
                throw std::runtime_error{std::move(msg).str()};
            }

            return *ptr;
        }

        MIObject const* implFind(UID id) const final
        {
            return findByID(m_Objects, id);
        }

        void populateDeletionSet(const MIObject& deletionTarget, std::unordered_set<UID>& out)
        {
            UID const deletedID = deletionTarget.getID();

            // add the deletion target to the deletion set (if applicable)
            if (deletionTarget.canDelete())
            {
                if (!out.emplace(deletedID).second)
                {
                    throw std::runtime_error{"cannot populate deletion set - cycle detected"};
                }
            }

            // iterate over everything else in the document and look for things
            // that cross-reference the to-be-deleted object - those things should
            // also be deleted
            for (const MIObject& obj : iter())
            {
                if (obj.isCrossReferencing(deletedID))
                {
                    populateDeletionSet(obj, out);
                }
            }
        }

        ObjectLookup m_Objects;
        std::unordered_set<UID> m_SelectedObjectIDs;
        std::vector<ClonePtr<MIObject>> m_DeletedObjects;
    };
}
