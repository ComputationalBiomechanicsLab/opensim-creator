#pragma once

#include "oscar/Utils/CStringView.hpp"

#include <chrono>
#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

// undo/redo algorithm support
//
// snapshot-based, rather than command-pattern based. Designed to be reference-counted, and
// allow for implementations that don't need to know what, or how, the data is actually
// stored in memory
namespace osc
{
    // an abstract base class for storing undo/redo metadata
    class UndoRedoEntryMetadata {
    protected:
        UndoRedoEntryMetadata(std::string_view message_);
        UndoRedoEntryMetadata(UndoRedoEntryMetadata const&);
        UndoRedoEntryMetadata(UndoRedoEntryMetadata&&) noexcept;
        UndoRedoEntryMetadata& operator=(UndoRedoEntryMetadata const&);
        UndoRedoEntryMetadata& operator=(UndoRedoEntryMetadata&&) noexcept;
    public:
        virtual ~UndoRedoEntryMetadata() noexcept;

        std::chrono::system_clock::time_point getTime() const
        {
            return m_Time;
        }

        osc::CStringView getMessage() const
        {
            return m_Message;
        }
    private:
        std::chrono::system_clock::time_point m_Time;
        std::string m_Message;
    };

    // concrete implementation of storage for a complete undo/redo entry (metadata + data)
    template<typename T>
    class UndoRedoEntryData final : public UndoRedoEntryMetadata {
    public:
        template<typename... TCtorArgs>
        UndoRedoEntryData(std::string_view message_, TCtorArgs&&... args) :
            UndoRedoEntryMetadata{std::move(message_)},
            m_Data{std::forward<TCtorArgs>(args)...}
        {
        }

        T const& getData() const
        {
            return m_Data;
        }

    private:
        T m_Data;
    };

    // type-erased, const, and reference-counted storage for undo/redo entry data
    //
    // can be safely copied, sliced, etc. from the derived class, enabling type-erased
    // implementation code
    class UndoRedoEntry {
    protected:
        UndoRedoEntry(std::shared_ptr<UndoRedoEntryMetadata const> data_) :
            m_Data{std::move(data_)}
        {
        }

    public:
        std::chrono::system_clock::time_point getTime() const
        {
            return m_Data->getTime();
        }

        osc::CStringView getMessage() const
        {
            return m_Data->getMessage();
        }

    protected:
        std::shared_ptr<UndoRedoEntryMetadata const> m_Data;
    };

    // concrete, known-to-hold-type-T version of `UndoRedoEntry`
    template<typename T>
    class UndoRedoEntryT final : public UndoRedoEntry {
    public:
        template<typename... TCtorArgs>
        UndoRedoEntryT(std::string_view message_, TCtorArgs&&... args) :
            UndoRedoEntry{std::make_shared<UndoRedoEntryData<T>>(std::move(message_), std::forward<TCtorArgs>(args)...)}
        {
        }

        T const& getData() const
        {
            return static_cast<UndoRedoEntryData<T> const&>(*m_Data).getData();
        }
    };

    // type-erased base class for undo/redo storage
    //
    // this base class stores undo/redo entries as type-erased pointers, so that the
    // code here, and in other generic downstream classes, doesn't need to know what's
    // actually being stored
    class UndoRedo {
    protected:
        UndoRedo(UndoRedoEntry const& initialCommit_);
        UndoRedo(UndoRedo const&);
        UndoRedo(UndoRedo&&) noexcept;
        UndoRedo& operator=(UndoRedo const&);
        UndoRedo& operator=(UndoRedo&&) noexcept;

    public:
        virtual ~UndoRedo() noexcept;

        void commitScratch(std::string_view commitMsg);
        UndoRedoEntry const& getHead() const;

        size_t getNumUndoEntries() const;
        ptrdiff_t getNumUndoEntriesi() const;
        UndoRedoEntry const& getUndoEntry(ptrdiff_t i) const;
        void undoTo(ptrdiff_t nthEntry);
        bool canUndo() const;
        void undo();

        size_t getNumRedoEntries() const;
        ptrdiff_t getNumRedoEntriesi() const;
        UndoRedoEntry const& getRedoEntry(ptrdiff_t i) const;
        bool canRedo() const;
        void redoTo(ptrdiff_t nthEntry);
        void redo();

    private:
        virtual UndoRedoEntry implCreateCommitFromScratch(std::string_view commitMsg) = 0;
        virtual void implAssignScratchFromCommit(UndoRedoEntry const&) = 0;

        std::vector<UndoRedoEntry> m_Undo;
        std::vector<UndoRedoEntry> m_Redo;
        UndoRedoEntry m_Head;
    };

    // concrete class for undo/redo storage
    //
    // - there is a "scratch" space that other code can edit
    // - other code can "commit" the scratch space to storage via `commit(message)`
    // - there is always at least one commit (the "head") in storage, for rollback support
    template<typename T>
    class UndoRedoT final : public UndoRedo {
    public:
        template<typename... TCtorArgs>
        UndoRedoT(TCtorArgs&&... args) :
            UndoRedo(UndoRedoEntryT<T>{"created document", std::forward<TCtorArgs>(args)...}),
            m_Scratch{static_cast<UndoRedoEntryT<T> const&>(getHead()).getData()}
        {
        }

        T const& getScratch() const
        {
            return m_Scratch;
        }

        T& updScratch()
        {
            return m_Scratch;
        }

        UndoRedoEntryT<T> const& getUndoEntry(ptrdiff_t i) const
        {
            return static_cast<UndoRedoEntryT<T> const&>(static_cast<UndoRedo const&>(*this).getUndoEntry(i));
        }

        UndoRedoEntryT<T> const& getRedoEntry(ptrdiff_t i) const
        {
            return static_cast<UndoRedoEntryT<T> const&>(static_cast<UndoRedo const&>(*this).getRedoEntry(i));
        }

    private:
        virtual UndoRedoEntry implCreateCommitFromScratch(std::string_view commitMsg)
        {
            return UndoRedoEntryT<T>{std::move(commitMsg), m_Scratch};
        }

        virtual void implAssignScratchFromCommit(UndoRedoEntry const& commit)
        {
            m_Scratch = static_cast<UndoRedoEntryT<T> const&>(commit).getData();
        }

        T m_Scratch;
    };
}