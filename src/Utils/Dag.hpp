#pragma once

#include "src/Utils/UID.hpp"
#include "src/Assertions.hpp"

#include <chrono>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace osc::experimental
{
    using CommitClock = std::chrono::system_clock;

    // the actual data held within a commit
    template<class T>
    class CommitData final {
    public:
        template<class... Args>
        CommitData(std::string_view commitMessage, Args&&... args) :
            m_CommitMessage{std::move(commitMessage)},
            m_Data{std::forward<Args>(args)...}
        {
        }

        template<class... Args>
        CommitData(UID parent, std::string_view commitMessage, Args&&... args) :
            m_MaybeParentID{std::move(parent)},
            m_CommitMessage{std::move(commitMessage)},
            m_Data{std::forward<Args>(args)...}
        {
        }

        UID getID() const { return m_ID; }
        bool hasParent() const { return m_ID != UID::empty(); }
        UID getParentID() const { return m_MaybeParentID; }
        CommitClock::time_point getCreationTime() const { return m_CreationTime; }
        std::string const& getCommitMessage() const { return m_CommitMessage; }
        T const& getData() const { return m_Data; }

    private:
        // unique ID for this commit
        UID m_ID;

        // (maybe) unique ID of the parent commit
        UID m_MaybeParentID = UID::empty();

        // when this commit object was created
        CommitClock::time_point m_CreationTime = CommitClock::now();

        // commit message
        std::string m_CommitMessage;

        // the (immutable) data saved in the commit
        T m_Data;
    };

    // operators for `CommitData`
    //
    // note: a commit ID is globally unique because of the guarantees offered
    //       by `osc::UID`, and it can't be externally assigned, so every unique
    //       instance of `osc::CommitData` can only have the same UID if it is the
    //       same, or a copy-constructed/copy-assigned, instance

    template<typename T>
    bool operator==(CommitData<T> const& a, CommitData<T> const& b)
    {
        return a.getID() == b.getID();
    }

    template<typename T>
    bool operator!=(CommitData<T> const& a, CommitData<T> const& b)
    {
        return a.getID() != b.getID();
    }

    template<typename T>
    bool operator<(CommitData<T> const& a, CommitData<T> const& b)
    {
        return a.getID() < b.getID();
    }

    template<typename T>
    bool operator<=(CommitData<T> const& a, CommitData<T> const& b)
    {
        return a.getID() <= b.getID();
    }

    template<typename T>
    bool operator>(CommitData<T> const& a, CommitData<T> const& b)
    {
        return a.getID() > b.getID();
    }

    template<typename T>
    bool operator>=(CommitData<T> const& a, CommitData<T> const& b)
    {
        return a.getID() >= b.getID();
    }
}

namespace std
{
    template<class T>
    struct hash<osc::experimental::CommitData<T>> {
        std::size_t operator()(osc::experimental::CommitData<T> const& c) const
        {
            return std::hash<osc::UID>{}(c.getID());
        }
    };
}

namespace osc::experimental
{
    // reference-counted commit "value" that can be used to access the
    // immutable commit data
    template<class T>
    class Commit {
    public:
        template<class... Args>
        Commit(std::string_view commitMessage, Args&&... args) :
            m_Handle{std::make_shared<CommitData<T>>(std::move(commitMessage), std::forward<Args>(args)...)}
        {
        }

        template<class... Args>
        Commit(UID parent, std::string_view commitMessage, Args&&... args) :
            m_Handle{std::make_shared<CommitData<T>>(std::move(parent), std::move(commitMessage), std::forward<Args>(args)...)}
        {
        }

        UID getID() const { return m_Handle->getID(); }
        bool hasParent() const { return m_Handle->hasParent(); }
        UID getParentID() const { return m_Handle->getParentID(); }
        CommitClock::time_point getCreationTime() const { return m_Handle->getCreationTime(); }
        std::string const& getCommitMessage() const { return m_Handle->getCommitMessage(); }
        T const& getData() const { return m_Handle->getData(); }

    private:
        std::shared_ptr<CommitData<T>> m_Handle;
    };

    template<class T>
    bool operator==(Commit<T> const& a, Commit<T> const& b)
    {
        return a.getID() == b.getID();
    }

    template<class T>
    bool operator!=(Commit<T> const& a, Commit<T> const& b)
    {
        return a.getID() != b.getID();
    }

    template<class T>
    bool operator<(Commit<T> const& a, Commit<T> const& b)
    {
        return a.getID() < b.getID();
    }
    template<class T>
    bool operator<=(Commit<T> const& a, Commit<T> const& b)
    {
        return a.getID() <= b.getID();
    }

    template<class T>
    bool operator>(Commit<T> const& a, Commit<T> const& b)
    {
        return a.getID() > b.getID();
    }

    template<class T>
    bool operator>=(Commit<T> const& a, Commit<T> const& b)
    {
        return a.getID() == b.getID();
    }
}

namespace std
{
    template<class T>
    struct hash<osc::experimental::Commit<T>> {
        std::size_t operator()(osc::experimental::Commit<T> const& c) const
        {
            return std::hash<osc::UID>{}(c.getID());
        }
    };
}

namespace osc::experimental
{
    template<class T>
    class Dag final {
    public:
        template<class... Args>
        Dag(Args&&... args) : m_Scratch{std::forward<Args>(args)...}
        {
            commit("initial commit");  // make initial commit
        }

        Commit<T> commit(std::string_view commitMessage)
        {
            auto commit = Commit<T>{std::move(commitMessage), m_Scratch};

            m_Commits.try_emplace(commit.getID(), commit);
            m_CurrentHead = commit.getID();
            m_BranchHead = commit.getID();

            return commit;
        }

        std::optional<Commit<T>> tryGetCommitByID(UID id) const
        {
            auto it = m_Commits.find(id);
            return it != m_Commits.end() ? it->second : std::nullopt;
        }

        bool hasCommit(UID id) const
        {
            return tryGetCommitByID(std::move(id));
        }

        Commit<T> getHeadCommit() const
        {
            OSC_ASSERT(m_CurrentHead != UID::empty());
            OSC_ASSERT(hasCommit(m_CurrentHead));

            return *tryGetCommitByID(m_CurrentHead);
        }

        UID getHeadCommitID() const
        {
            return m_CurrentHead;
        }

        void checkout()
        {
            m_Scratch = getHeadCommit().getValue();
        }

        T& updScratch()
        {
            return m_Scratch;
        }

        T const& getScratch() const
        {
            return m_Scratch;
        }

        // remove out-of-bounds, deleted, out-of-date, etc. commits
        void garbageCollect()
        {
            garbageCollectMaxUndo();
            garbageCollectMaxRedo();
            garbageCollectUnreachable();
        }

    private:
        // try to lookup the *parent* of a given commit, or return an empty (senteniel) ID
        UID tryGetParentIDOrEmpty(UID id) const
        {
            auto commit = tryGetCommitByID(id);
            return commit ? commit->getParentID() : UID::empty();
        }

        // returns the number of hops between commit `a` and commit `b`
        //
        // returns -1 if commit `b` cannot be reached from commit `a`
        int distance(UID a, UID b) const
        {
            if (a == b)
            {
                return 0;
            }

            int n = 1;
            UID parent = tryGetParentIDOrEmpty(a);

            while (parent != b && parent != UID::empty())
            {
                parent = tryGetParentIDOrEmpty(parent);
                n++;
            }

            return parent == b ? n : -1;
        }

        // returns a pointer to a commit that is the nth ancestor from `a`
        //
        // (e.g. n==0 returns `a`, n==1 returns `a`'s parent, n==2 returns `a`'s grandparent)
        //
        // returns `nullptr` if there are insufficient ancestors. `n` must be >= 0
        Commit<T> const* nthAncestor(UID a, int n) const
        {
            if (n < 0)
            {
                return nullptr;
            }

            Commit<T> const* c = tryGetCommitByID(a);

            if (!c || n == 0)
            {
                return c;
            }

            int i = 1;
            c = tryGetCommitByID(c->getParentID());

            while (c && i < n)
            {
                c = tryGetCommitByID(c->getParentID());
                i++;
            }

            return c;
        }

        // returns the UID that is the nth ancestor from `a`, or empty if there are insufficient ancestors
        UID nthAncestorID(UID a, int n) const
        {
            Commit<T> const* c = nthAncestor(a, n);
            return c ? c->getID() : UID::empty();
        }

        // returns `true` if `maybeAncestor` is an ancestor of `id`
        bool isAncestor(UID maybeAncestor, UID id)
        {
            Commit<T> const* c = tryGetCommitByID(id);

            while (c && c->getID() != maybeAncestor)
            {
                c = tryGetCommitByID(c->getParentID());
            }

            return c;
        }

        // remove a range of commits from `start` (inclusive) to `end` (exclusive)
        void eraseCommitRange(UID start, UID end)
        {
            auto it = m_Commits.find(start);

            while (it != m_Commits.end() && it->second.getID() != end)
            {
                UID parent = it->second.getParentID();
                m_Commits.erase(it);

                it = m_Commits.find(parent);
            }
        }

        // garbage collect (erase) commits that fall outside the maximum undo depth
        void garbageCollectMaxUndo()
        {
            static_assert(m_MaxUndo >= 0);

            UID firstBadCommitIDOrEmpty = nthAncestorID(m_CurrentHead, m_MaxUndo + 1);
            eraseCommitRange(firstBadCommitIDOrEmpty, UID::empty());
        }

        // garbage collect (erase) commits that fall outside the maximum redo depth
        void garbageCollectMaxRedo()
        {
            static_assert(m_MaxRedo >= 0);

            int numRedos = distance(m_BranchHead, m_CurrentHead);
            int numDeletions = numRedos - m_MaxRedo;

            if (numDeletions <= 0)
            {
                return;
            }

            UID newBranchHead = nthAncestorID(m_BranchHead, numDeletions);
            eraseCommitRange(m_BranchHead, newBranchHead);
            m_BranchHead = newBranchHead;
        }

        void garbageCollectUnreachable()
        {
            for (auto it = m_Commits.begin(); it != m_Commits.end();)
            {
                if (it->first == m_BranchHead || isAncestor(it->first, m_BranchHead))
                {
                    ++it;
                }
                else
                {
                    it = m_Commits.erase(it);
                }
            }
        }

        // mutable staging area that calling code can mutate
        T m_Scratch;

        // where scratch will commit to (i.e. the parent of the scratch area)
        UID m_CurrentHead = UID::empty();

        // head of the current branch (i.e. "master") - may be ahead of current branch (undo/redo)
        UID m_BranchHead = UID::empty();

        // maximum distance between the current commit and the "root" commit (i.e. a commit with no parent)
        static constexpr int m_MaxUndo = 32;

        // maximum distance between the branch head and the current commit (i.e. how big the redo buffer can be)
        static constexpr int m_MaxRedo = 32;

        // underlying storage for immutable commits
        std::unordered_map<UID, Commit<T>> m_Commits;
    };
}
