#pragma once

#include <OpenSimCreator/Documents/MeshImporter/IObjectFinder.hpp>
#include <OpenSimCreator/Documents/MeshImporter/MIObject.hpp>

#include <oscar/Utils/ClonePtr.hpp>
#include <oscar/Utils/Concepts.hpp>
#include <oscar/Utils/UID.hpp>

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
    // scene elements are collected into a single, potentially interconnected, model graph
    // datastructure. This datastructure is what ultimately maps into an "OpenSim::Model".
    //
    // Main design considerations:
    //
    // - Must have somewhat fast associative lookup semantics, because the UI needs to
    //   traverse the graph in a value-based (rather than pointer-based) way
    //
    // - Must have value semantics, so that other code such as the undo/redo buffer can
    //   copy an entire ModelGraph somewhere else in memory without having to worry about
    //   aliased mutations
    class Document final : public IObjectFinder {

        using ObjectLookup = std::map<UID, ClonePtr<MIObject>>;

        // helper class for iterating over model graph elements
        template<DerivedFrom<MIObject> T>
        class Iterator final {
        public:
            using difference_type = ptrdiff_t;
            using value_type = T;
            using pointer = T*;
            using const_pointer = T const*;
            using reference = T const&;
            using iterator_category = std::input_iterator_tag;

            // caller-provided iterator
            using InternalIterator = std::conditional_t<
                std::is_const_v<T>,
                ObjectLookup::const_iterator,
                ObjectLookup::iterator
            >;

            Iterator(InternalIterator pos_, InternalIterator end_) :
                m_Pos{pos_},
                m_End{end_}
            {
                // ensure iterator initially points at an element with the correct type
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

            reference operator*() const
            {
                return dynamic_cast<reference>(*m_Pos->second);
            }

            // EqualityComparable

            friend bool operator==(Iterator const&, Iterator const&) = default;

            // LegacyInputIterator

            pointer operator->() const
            {

                return &dynamic_cast<reference>(*m_Pos->second);
            }

        private:
            InternalIterator m_Pos;
            InternalIterator m_End;
        };

        // helper class for an iterable object with a beginning + end
        template<DerivedFrom<MIObject> T>
        class Iterable final {
        public:
            using MapRef = std::conditional_t<std::is_const_v<T>, ObjectLookup const&, ObjectLookup&>;

            explicit Iterable(MapRef els) :
                m_Begin{els.begin(), els.end()},
                m_End{els.end(), els.end()}
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

        template<DerivedFrom<MIObject> T = MIObject>
        T* tryUpdByID(UID id)
        {
            return findElByID<T>(m_Els, id);
        }

        template<DerivedFrom<MIObject> T = MIObject>
        T const* tryGetByID(UID id) const
        {
            return findElByID<T>(m_Els, id);
        }

        template<DerivedFrom<MIObject> T = MIObject>
        T& updElByID(UID id)
        {
            return findElByIDOrThrow<T>(m_Els, id);
        }

        template<DerivedFrom<MIObject> T = MIObject>
        T const& getElByID(UID id) const
        {
            return findElByIDOrThrow<T>(m_Els, id);
        }

        template<DerivedFrom<MIObject> T = MIObject>
        bool containsEl(UID id) const
        {
            return tryGetByID<T>(id);
        }

        template<DerivedFrom<MIObject> T = MIObject>
        bool containsEl(MIObject const& e) const
        {
            return containsEl<T>(e.getID());
        }

        template<DerivedFrom<MIObject> T = MIObject>
        Iterable<T> iter()
        {
            return Iterable<T>{m_Els};
        }

        template<DerivedFrom<MIObject> T = MIObject>
        Iterable<T const> iter() const
        {
            return Iterable<T const>{m_Els};
        }

        MIObject& addEl(std::unique_ptr<MIObject> el)
        {
            // ensure element connects to things that already exist in the model
            // graph

            for (int i = 0, len = el->getNumCrossReferences(); i < len; ++i)
            {
                if (!containsEl(el->getCrossReferenceConnecteeID(i)))
                {
                    std::stringstream ss;
                    ss << "cannot add '" << el->getLabel() << "' (ID = " << el->getID() << ") to model graph because it contains a cross reference (label = " << el->getCrossReferenceLabel(i) << ") to a scene element that does not exist in the model graph";
                    throw std::runtime_error{std::move(ss).str()};
                }
            }

            return *m_Els.emplace(el->getID(), std::move(el)).first->second;
        }

        template<DerivedFrom<MIObject> T, typename... Args>
        T& emplaceEl(Args&&... args) requires ConstructibleFrom<T, Args&&...>
        {
            return static_cast<T&>(addEl(std::make_unique<T>(std::forward<Args>(args)...)));
        }

        bool deleteElByID(UID id)
        {
            MIObject const* const el = tryGetByID(id);

            if (!el)
            {
                return false;  // ID doesn't exist in the model graph
            }

            // collect all to-be-deleted elements into one deletion set so that the deletion
            // happens in separate phase from the "search for things to delete" phase
            std::unordered_set<UID> deletionSet;
            populateDeletionSet(*el, deletionSet);

            for (UID deletedID : deletionSet)
            {
                deSelect(deletedID);

                // move element into deletion set, rather than deleting it immediately,
                // so that code that relies on references to the to-be-deleted element
                // still works until an explicit `.GarbageCollect()` call

                auto const it = m_Els.find(deletedID);
                if (it != m_Els.end())
                {
                    m_DeletedEls.push_back(std::move(it->second));
                    m_Els.erase(it);
                }
            }

            return !deletionSet.empty();
        }

        bool deleteEl(MIObject const& el)
        {
            return deleteElByID(el.getID());
        }

        void garbageCollect()
        {
            m_DeletedEls.clear();
        }


        // selection logic

        std::unordered_set<UID> const& getSelected() const
        {
            return m_SelectedEls;
        }

        bool isSelected(UID id) const
        {
            return m_SelectedEls.find(id) != m_SelectedEls.end();
        }

        bool isSelected(MIObject const& el) const
        {
            return isSelected(el.getID());
        }

        void select(UID id)
        {
            MIObject const* const e = tryGetByID(id);

            if (e && e->canSelect())
            {
                m_SelectedEls.insert(id);
            }
        }

        void select(MIObject const& el)
        {
            select(el.getID());
        }

        void deSelect(UID id)
        {
            m_SelectedEls.erase(id);
        }

        void deSelect(MIObject const& el)
        {
            deSelect(el.getID());
        }

        void selectAll()
        {
            for (MIObject const& e : iter())
            {
                if (e.canSelect())
                {
                    m_SelectedEls.insert(e.getID());
                }
            }
        }

        void deSelectAll()
        {
            m_SelectedEls.clear();
        }
    private:
        template<DerivedFrom<MIObject> T = MIObject, typename Container>
        static T* findElByID(Container& container, UID id)
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

        template<DerivedFrom<MIObject> T = MIObject, typename Container>
        static T& findElByIDOrThrow(Container& container, UID id)
        {
            T* ptr = findElByID<T>(container, id);
            if (!ptr)
            {
                std::stringstream msg;
                msg << "could not find a scene element of type " << typeid(T).name() << " with ID = " << id;
                throw std::runtime_error{std::move(msg).str()};
            }

            return *ptr;
        }

        MIObject const* implFind(UID id) const final
        {
            return findElByID(m_Els, id);
        }

        void populateDeletionSet(MIObject const& deletionTarget, std::unordered_set<UID>& out)
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

            // iterate over everything else in the model graph and look for things
            // that cross-reference the to-be-deleted element - those things should
            // also be deleted

            for (MIObject const& el : iter())
            {
                if (el.isCrossReferencing(deletedID))
                {
                    populateDeletionSet(el, out);
                }
            }
        }

        // insert a senteniel ground element into the model graph (it should always
        // be there)
        ObjectLookup m_Els;
        std::unordered_set<UID> m_SelectedEls;
        std::vector<ClonePtr<MIObject>> m_DeletedEls;
    };
}
