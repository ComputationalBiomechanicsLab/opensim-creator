#pragma once

#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/UID.h>

#include <chrono>
#include <concepts>
#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

// undo/redo algorithm support
//
// snapshot-based, rather than command-pattern based. Designed to be reference-counted, and
// type-eraseable, so that generic downstream code doesn't necessarily need to know what,
// or how, the data is actually stored in memory
namespace osc
{
    template<typename T>
    concept Undoable = std::destructible<T> and std::copy_constructible<T>;

    // internal storage details
    namespace detail
    {
        // a base class for storing undo/redo metadata
        class UndoRedoEntryMetadata {
        protected:
            explicit UndoRedoEntryMetadata(std::string_view message) :
                message_{message}
            {}
            UndoRedoEntryMetadata(const UndoRedoEntryMetadata&) = default;
            UndoRedoEntryMetadata(UndoRedoEntryMetadata&&) noexcept = default;
            UndoRedoEntryMetadata& operator=(const UndoRedoEntryMetadata&) = default;
            UndoRedoEntryMetadata& operator=(UndoRedoEntryMetadata&&) noexcept = default;
        public:
            virtual ~UndoRedoEntryMetadata() noexcept = default;

            UID id() const { return id_; }
            std::chrono::system_clock::time_point time() const { return time_; }
            CStringView message() const { return message_; }

        private:
            UID id_;
            std::chrono::system_clock::time_point time_ = std::chrono::system_clock::now();
            std::string message_;
        };

        // concrete implementation of storage for a complete undo/redo entry (metadata + value)
        template<Undoable T>
        class UndoRedoEntryData final : public UndoRedoEntryMetadata {
        public:
            template<typename... Args>
            requires std::constructible_from<T, Args&&...>
            UndoRedoEntryData(std::string_view message, Args&&... args) :
                UndoRedoEntryMetadata{std::move(message)},
                value_{std::forward<Args>(args)...}
            {}

            const T& value() const { return value_; }

        private:
            T value_;
        };
    }

    // type-erased, const, and reference-counted storage for undo/redo entry data
    //
    // can be safely copied, sliced, etc. from the derived class, enabling type-erased
    // implementation code
    class UndoRedoEntryBase {
    protected:
        explicit UndoRedoEntryBase(std::shared_ptr<const detail::UndoRedoEntryMetadata> data) :
            data_{std::move(data)}
        {}

    public:
        UID id() const { return data_->id(); }
        std::chrono::system_clock::time_point time() const { return data_->time(); }
        CStringView message() const { return data_->message(); }

    protected:
        std::shared_ptr<const detail::UndoRedoEntryMetadata> data_;
    };

    // concrete, known-to-hold-type-T version of `UndoRedoEntry`
    template<Undoable T>
    class UndoRedoEntry final : public UndoRedoEntryBase {
    public:
        template<typename... Args>
        requires std::constructible_from<T, Args&&...>
        UndoRedoEntry(std::string_view message, Args&&... args) :
            UndoRedoEntryBase{std::make_shared<detail::UndoRedoEntryData<T>>(std::move(message), std::forward<Args>(args)...)}
        {}

        const T& value() const { return static_cast<const detail::UndoRedoEntryData<T>&>(*data_).value(); }
    };

    // type-erased base class for undo/redo storage
    //
    // this base class stores undo/redo entries as type-erased pointers, so that the
    // code here, and in other generic downstream classes, doesn't need to know what's
    // actually being stored
    class UndoRedoBase {
    protected:
        explicit UndoRedoBase(UndoRedoEntryBase initial_commit);
        UndoRedoBase(const UndoRedoBase&);
        UndoRedoBase(UndoRedoBase&&) noexcept;
        UndoRedoBase& operator=(const UndoRedoBase&);
        UndoRedoBase& operator=(UndoRedoBase&&) noexcept;

    public:
        virtual ~UndoRedoBase() noexcept;

        void commit_scratch(std::string_view commit_message);
        const UndoRedoEntryBase& head() const;
        UID head_id() const;

        size_t num_undo_entries() const;
        ptrdiff_t num_undo_entriesi() const;
        const UndoRedoEntryBase& undo_entry_at(ptrdiff_t pos) const;
        void undo_to(ptrdiff_t pos);
        bool can_undo() const;
        void undo();

        size_t num_redo_entries() const;
        ptrdiff_t num_redo_entriesi() const;
        const UndoRedoEntryBase& redo_entry_at(ptrdiff_t pos) const;
        bool can_redo() const;
        void redo_to(ptrdiff_t pos);
        void redo();

    private:
        virtual UndoRedoEntryBase impl_construct_commit_from_scratch(std::string_view commit_message) = 0;
        virtual void impl_copy_assign_scratch_from_commit(const UndoRedoEntryBase&) = 0;

        std::vector<UndoRedoEntryBase> undo_;
        std::vector<UndoRedoEntryBase> redo_;
        UndoRedoEntryBase head_;
    };

    // concrete class for undo/redo storage
    //
    // - there is a "scratch" space that other code can edit
    // - other code can "commit" the scratch space to storage via `commit(message)`
    // - there is always at least one commit (the "head") in storage, for rollback support
    template<Undoable T>
    class UndoRedo final : public UndoRedoBase {
    public:
        template<typename... Args>
        requires std::constructible_from<T, Args&&...>
        UndoRedo(Args&&... args) :
            UndoRedoBase(UndoRedoEntry<T>{"created document", std::forward<Args>(args)...}),
            scratch_{static_cast<const UndoRedoEntry<T>&>(head()).value()}
        {}

        const T& scratch() const { return scratch_; }

        T& upd_scratch() { return scratch_; }

        const UndoRedoEntry<T>& undo_entry_at(ptrdiff_t pos) const
        {
            return static_cast<const UndoRedoEntry<T>&>(static_cast<const UndoRedoBase&>(*this).undo_entry_at(pos));
        }

        const UndoRedoEntry<T>& redo_entry_at(ptrdiff_t pos) const
        {
            return static_cast<const UndoRedoEntry<T>&>(static_cast<const UndoRedoBase&>(*this).redo_entry_at(pos));
        }

    private:
        virtual UndoRedoEntryBase impl_construct_commit_from_scratch(std::string_view commit_message)
        {
            return UndoRedoEntry<T>{std::move(commit_message), scratch_};
        }

        virtual void impl_copy_assign_scratch_from_commit(const UndoRedoEntryBase& commit)
        {
            scratch_ = static_cast<const UndoRedoEntry<T>&>(commit).value();
        }

        T scratch_;
    };
}
