#pragma once

#include <OpenSimCreator/ModelGraph/GroundEl.hpp>
#include <OpenSimCreator/ModelGraph/ISceneElLookup.hpp>
#include <OpenSimCreator/ModelGraph/ModelGraphIDs.hpp>
#include <OpenSimCreator/ModelGraph/SceneEl.hpp>

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

namespace osc
{
    // modelgraph support
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
    class ModelGraph final : public ISceneElLookup {

        using SceneElMap = std::map<UID, ClonePtr<SceneEl>>;

        // helper class for iterating over model graph elements
        template<DerivedFrom<SceneEl> T>
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
                SceneElMap::const_iterator,
                SceneElMap::iterator
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
        template<DerivedFrom<SceneEl> T>
        class Iterable final {
        public:
            using MapRef = std::conditional_t<std::is_const_v<T>, SceneElMap const&, SceneElMap&>;

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

        ModelGraph() = default;

        template<DerivedFrom<SceneEl> T = SceneEl>
        T* tryUpdElByID(UID id)
        {
            return findElByID<T>(m_Els, id);
        }

        template<DerivedFrom<SceneEl> T = SceneEl>
        T const* tryGetElByID(UID id) const
        {
            return findElByID<T>(m_Els, id);
        }

        template<DerivedFrom<SceneEl> T = SceneEl>
        T& updElByID(UID id)
        {
            return findElByIDOrThrow<T>(m_Els, id);
        }

        template<DerivedFrom<SceneEl> T = SceneEl>
        T const& getElByID(UID id) const
        {
            return findElByIDOrThrow<T>(m_Els, id);
        }

        template<DerivedFrom<SceneEl> T = SceneEl>
        bool containsEl(UID id) const
        {
            return tryGetElByID<T>(id);
        }

        template<DerivedFrom<SceneEl> T = SceneEl>
        bool containsEl(SceneEl const& e) const
        {
            return containsEl<T>(e.getID());
        }

        template<DerivedFrom<SceneEl> T = SceneEl>
        Iterable<T> iter()
        {
            return Iterable<T>{m_Els};
        }

        template<DerivedFrom<SceneEl> T = SceneEl>
        Iterable<T const> iter() const
        {
            return Iterable<T const>{m_Els};
        }

        SceneEl& addEl(std::unique_ptr<SceneEl> el)
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

        template<DerivedFrom<SceneEl> T, typename... Args>
        T& emplaceEl(Args&&... args) requires ConstructibleFrom<T, Args&&...>
        {
            return static_cast<T&>(addEl(std::make_unique<T>(std::forward<Args>(args)...)));
        }

        bool deleteElByID(UID id)
        {
            SceneEl const* const el = tryGetElByID(id);

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

        bool deleteEl(SceneEl const& el)
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

        bool isSelected(SceneEl const& el) const
        {
            return isSelected(el.getID());
        }

        void select(UID id)
        {
            SceneEl const* const e = tryGetElByID(id);

            if (e && e->canSelect())
            {
                m_SelectedEls.insert(id);
            }
        }

        void select(SceneEl const& el)
        {
            select(el.getID());
        }

        void deSelect(UID id)
        {
            m_SelectedEls.erase(id);
        }

        void deSelect(SceneEl const& el)
        {
            deSelect(el.getID());
        }

        void selectAll()
        {
            for (SceneEl const& e : iter())
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
        template<DerivedFrom<SceneEl> T = SceneEl, typename Container>
        static T* findElByID(Container& container, UID id)
        {
            auto const it = container.find(id);

            if (it == container.end())
            {
                return nullptr;
            }

            if constexpr (std::is_same_v<T, SceneEl>)
            {
                return it->second.get();
            }
            else
            {
                return dynamic_cast<T*>(it->second.get());
            }
        }

        template<DerivedFrom<SceneEl> T = SceneEl, typename Container>
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

        SceneEl const* implFind(UID id) const final
        {
            return findElByID(m_Els, id);
        }

        void populateDeletionSet(SceneEl const& deletionTarget, std::unordered_set<UID>& out)
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

            for (SceneEl const& el : iter())
            {
                if (el.isCrossReferencing(deletedID))
                {
                    populateDeletionSet(el, out);
                }
            }
        }

        // insert a senteniel ground element into the model graph (it should always
        // be there)
        SceneElMap m_Els = {{ModelGraphIDs::Ground(), ClonePtr<SceneEl>{GroundEl{}}}};
        std::unordered_set<UID> m_SelectedEls;
        std::vector<ClonePtr<SceneEl>> m_DeletedEls;
    };
}
