#include "undoable_model_state_pair.h"

#include <libopensimcreator/documents/model/environment.h>
#include <libopensimcreator/documents/model/model_state_commit.h>

#include <libopynsim/utilities/open_sim_helpers.h>
#include <liboscar/platform/log.h>
#include <liboscar/utilities/algorithms.h>
#include <liboscar/utilities/assertions.h>
#include <liboscar/utilities/perf.h>
#include <liboscar/utilities/uid.h>
#include <OpenSim/Common/ComponentPath.h>
#include <OpenSim/Common/ModelDisplayHints.h>
#include <OpenSim/Common/PropertyObjArray.h>
#include <OpenSim/Simulation/Model/Model.h>

#include <exception>
#include <filesystem>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace osc;

namespace
{
    // maximum distance between the current commit and the "root" commit (i.e. a commit with no
    // parent)
    inline constexpr int c_max_undo = 32;

    // maximum distance between the branch head and the current commit (i.e. how big the redo buffer
    // can be)
    inline constexpr int c_max_redo = 32;

    std::unique_ptr<OpenSim::Model> make_new_model()
    {
        auto rv = std::make_unique<OpenSim::Model>();
        rv->updDisplayHints().set_show_frames(true);
        return rv;
    }

    class UiModelStatePair final : public opyn::ModelStatePair {
    public:
        UiModelStatePair() :
            UiModelStatePair{make_new_model()}
        {}

        explicit UiModelStatePair(const std::string& osim) :
            UiModelStatePair{opyn::load_model(osim)}
        {}

        explicit UiModelStatePair(std::unique_ptr<OpenSim::Model> model) :
            model_{std::move(model)},
            fixup_scale_factor_{1.0f}
        {
            opyn::initialize_model(*model_);
            opyn::initialize_state(*model_);
        }

        UiModelStatePair(const UiModelStatePair& other) :
            model_{std::make_unique<OpenSim::Model>(*other.model_)},
            fixup_scale_factor_{other.fixup_scale_factor_},
            maybe_selected_{other.maybe_selected_},
            maybe_hovered_{other.maybe_hovered_}
        {
            opyn::initialize_model(*model_);
            opyn::initialize_state(*model_);
        }

        UiModelStatePair(UiModelStatePair&&) noexcept = default;
        UiModelStatePair& operator=(const UiModelStatePair&) = delete;
        UiModelStatePair& operator=(UiModelStatePair&&) noexcept = default;
        ~UiModelStatePair() noexcept override = default;

        const OpenSim::Model& impl_get_model() const override { return *model_; }

        OpenSim::Model& upd_model()
        {
            model_version_ = UID{};
            return *model_;
        }

        UID impl_get_model_version() const override { return model_version_; }

        void set_model_version(UID version) { model_version_ = version; }

        const SimTK::State& impl_get_state() const override { return model_->getWorkingState(); }

        UID impl_get_state_version() const override { return model_version_; }

        float impl_get_fixup_scale_factor() const override { return fixup_scale_factor_; }

        void impl_set_fixup_scale_factor(float sf) override { fixup_scale_factor_ = sf; }

        const OpenSim::ComponentPath& get_selected_path() const { return maybe_selected_; }

        void set_selected_path(const OpenSim::ComponentPath& p) { maybe_selected_ = p; }

        const OpenSim::Component* impl_get_selected() const override
        {
            return opyn::find_component(*model_, maybe_selected_);
        }

        void impl_set_selected(const OpenSim::Component* c) override
        {
            maybe_selected_ = opyn::get_absolute_path_or_empty(c);
        }

        const OpenSim::ComponentPath& get_hovered_path() const { return maybe_hovered_; }

        void set_hovered_path(const OpenSim::ComponentPath& p) { maybe_hovered_ = p; }

        const OpenSim::Component* impl_get_hovered() const override
        {
            return opyn::find_component(*model_, maybe_hovered_);
        }

        void impl_set_hovered(const OpenSim::Component* c) override
        {
            maybe_hovered_ = opyn::get_absolute_path_or_empty(c);
        }

    private:
        // the model, finalized from its properties
        std::unique_ptr<OpenSim::Model> model_;
        UID model_version_;

        // fixup scale factor of the model
        //
        // this scales up/down the decorations of the model - used for extremely
        // undersized models (e.g. fly leg)
        float fixup_scale_factor_;

        // (maybe) absolute path to the current selection (empty otherwise)
        OpenSim::ComponentPath maybe_selected_;

        // (maybe) absolute path to the current hover (empty otherwise)
        OpenSim::ComponentPath maybe_hovered_;
    };

    void copy_selected_and_hovered(const UiModelStatePair& src, UiModelStatePair& dest)
    {
        dest.set_selected_path(src.get_selected_path());
        dest.set_hovered_path(src.get_hovered_path());
    }
}

class osc::UndoableModelStatePair::Impl final {
public:
    Impl()
    {
        do_commit("created a new model");  // make initial commit
    }

    explicit Impl(const OpenSim::Model& m) :
        Impl{std::make_unique<OpenSim::Model>(m)}
    {}

    // crete a new commit graph that contains a backup of the given model
    explicit Impl(std::unique_ptr<OpenSim::Model> m) :
        scratch_{std::move(m)}
    {
        std::stringstream ss;
        if (auto input_path = opyn::try_find_input_file(get_model())) {
            ss << "loaded " << input_path->filename().string();
        } else {
            ss << "loaded model";
        }
        do_commit(std::move(ss).str());  // make initial commit
    }

    explicit Impl(const std::filesystem::path& osim_path) :
        Impl{opyn::load_model(osim_path)}
    {
        set_up_to_date_with_filesystem(std::filesystem::last_write_time(osim_path));
    }

    bool is_up_to_date_with_filesystem() const
    {
        return get_checkout_id() == get_filesystem_version();
    }

    void set_up_to_date_with_filesystem(std::filesystem::file_time_type t)
    {
        maybe_filesystem_timestamp_ = t;
        maybe_commit_saved_to_disk_ = current_head_;
    }

    std::filesystem::file_time_type get_last_filesystem_write_time() const
    {
        return maybe_filesystem_timestamp_;
    }

    ModelStateCommit get_latest_commit() const { return get_head_commit(); }

    bool can_undo() const
    {
        const ModelStateCommit* c = try_get_commit_by_id(current_head_);
        return c ? has_commit(c->get_parent_id()) : false;
    }

    void do_undo()
    {
        if (can_undo()) {
            undo();
        }
    }

    bool can_redo() const { return distance(branch_head_, current_head_) > 0; }

    void do_redo()
    {
        if (can_redo()) {
            redo();
        }
    }

    void commit(std::string_view message)
    {
        OSC_PERF("commit model");
        do_commit(message);
    }

    void rollback()
    {
        checkout();  // care: skip copying selection because a rollback is aggro
    }

    bool try_checkout(const ModelStateCommit& commit)
    {
        if (not commits_.contains(commit.get_id())) {
            return false;  // commit isn't in this model's storage (is it from another model?)
        }

        current_head_ = commit.get_id();
        checkout();
        return true;
    }

    const OpenSim::Model& get_model() const { return scratch_.get_model(); }

    OpenSim::Model& upd_model() { return scratch_.upd_model(); }

    void set_model(std::unique_ptr<OpenSim::Model> new_model)
    {
        UiModelStatePair p{std::move(new_model)};
        copy_selected_and_hovered(scratch_, p);
        p.set_fixup_scale_factor(scratch_.get_fixup_scale_factor());
        scratch_ = std::move(p);
    }

    void reset_model()
    {
        UiModelStatePair p;
        copy_selected_and_hovered(scratch_, p);
        p.set_fixup_scale_factor(scratch_.get_fixup_scale_factor());
        scratch_ = std::move(p);
    }

    void load_model(const std::filesystem::path& path)
    {
        set_model(opyn::load_model(path));
        set_up_to_date_with_filesystem(std::filesystem::last_write_time(path));
    }

    UID get_model_version() const { return scratch_.get_model_version(); }

    void set_model_version(UID version) { scratch_.set_model_version(version); }

    const SimTK::State& get_state() const { return scratch_.get_state(); }

    UID get_state_version() const { return scratch_.get_state_version(); }

    float get_fixup_scale_factor() const { return scratch_.get_fixup_scale_factor(); }

    void set_fixup_scale_factor(float v) { scratch_.set_fixup_scale_factor(v); }

    const OpenSim::Component* get_selected() const { return scratch_.get_selected(); }

    void set_selected(const OpenSim::Component* c) { scratch_.set_selected(c); }

    const OpenSim::Component* get_hovered() const { return scratch_.get_hovered(); }

    void set_hovered(const OpenSim::Component* c) { scratch_.set_hovered(c); }

    std::shared_ptr<Environment> impl_upd_associated_environment() { return environment_; }

private:
    UID do_commit(std::string_view message)
    {
        auto commit = ModelStateCommit{scratch_, message, current_head_};
        UID commit_id = commit.get_id();

        commits_.try_emplace(commit_id, std::move(commit));
        current_head_ = commit_id;
        branch_head_ = commit_id;

        garbage_collect();

        return commit_id;
    }

    // try to lookup a commit by its ID
    const ModelStateCommit* try_get_commit_by_id(UID id) const
    {
        return lookup_or_nullptr(commits_, id);
    }

    const ModelStateCommit& get_head_commit() const
    {
        OSC_ASSERT(current_head_ != UID::empty());
        OSC_ASSERT(has_commit(current_head_));

        return *try_get_commit_by_id(current_head_);
    }

    // try to lookup the *parent* of a given commit, or return an empty (senteniel) ID
    UID try_get_parent_id_or_empty(UID id) const
    {
        const ModelStateCommit* commit = try_get_commit_by_id(id);
        return commit ? commit->get_parent_id() : UID::empty();
    }

    // returns `true` if a commit with the given ID has been stored
    bool has_commit(UID id) const { return try_get_commit_by_id(id) != nullptr; }

    // returns the number of hops between commit `a` and commit `b`
    //
    // returns -1 if commit `b` cannot be reached from commit `a`
    int distance(UID a, UID b) const
    {
        if (a == b) {
            return 0;
        }

        int n = 1;
        UID parent = try_get_parent_id_or_empty(a);

        while (parent != b && parent != UID::empty()) {
            parent = try_get_parent_id_or_empty(parent);
            n++;
        }

        return parent == b ? n : -1;
    }

    // returns a pointer to a commit that is the nth ancestor from `a`
    //
    // (e.g. n==0 returns `a`, n==1 returns `a`'s parent, n==2 returns `a`'s grandparent)
    //
    // returns `nullptr` if there are insufficient ancestors. `n` must be >= 0
    const ModelStateCommit* nth_ancestor(UID a, int n) const
    {
        if (n < 0) {
            return nullptr;
        }

        const ModelStateCommit* c = try_get_commit_by_id(a);

        if (!c || n == 0) {
            return c;
        }

        int i = 1;
        c = try_get_commit_by_id(c->get_parent_id());

        while (c && i < n) {
            c = try_get_commit_by_id(c->get_parent_id());
            i++;
        }

        return c;
    }

    // returns the UID that is the nth ancestor from `a`, or empty if there are insufficient
    // ancestors
    UID nth_ancestor_id(UID a, int n) const
    {
        const ModelStateCommit* c = nth_ancestor(a, n);
        return c ? c->get_id() : UID::empty();
    }

    // returns `true` if `maybeAncestor` is an ancestor of `id`
    bool is_ancestor(UID maybe_ancestor, UID id)
    {
        const ModelStateCommit* c = try_get_commit_by_id(id);

        while (c != nullptr && c->get_id() != maybe_ancestor) {
            c = try_get_commit_by_id(c->get_parent_id());
        }

        return c != nullptr;
    }

    // remove a range of commits from `start` (inclusive) to `end` (exclusive)
    void erase_commit_range(UID start, UID end)
    {
        auto it = commits_.find(start);

        while (it != commits_.end() && it->second.get_id() != end) {
            UID parent = it->second.get_parent_id();
            commits_.erase(it);

            it = commits_.find(parent);
        }
    }

    // garbage collect (erase) commits that fall outside the maximum undo depth
    void garbage_collect_max_undo()
    {
        static_assert(c_max_undo >= 0);

        UID first_bad_commit_id_or_empty = nth_ancestor_id(current_head_, c_max_undo + 1);
        erase_commit_range(first_bad_commit_id_or_empty, UID::empty());
    }

    // garbage collect (erase) commits that fall outside the maximum redo depth
    void garbage_collect_max_redo()
    {
        static_assert(c_max_redo >= 0);

        int num_redos = distance(branch_head_, current_head_);
        int num_deletions = num_redos - c_max_redo;

        if (num_deletions <= 0) {
            return;
        }

        UID new_branch_head = nth_ancestor_id(branch_head_, num_deletions);
        erase_commit_range(branch_head_, new_branch_head);
        branch_head_ = new_branch_head;
    }

    void garbage_collect_unreachable()
    {
        std::erase_if(commits_, [this](const auto& p) {
            return !(p.first == branch_head_ || is_ancestor(p.first, branch_head_));
        });
    }

    // remove out-of-bounds, deleted, out-of-date, etc. commits
    void garbage_collect()
    {
        garbage_collect_max_undo();
        garbage_collect_max_redo();
        garbage_collect_unreachable();
    }

    // returns commit ID of the currently active checkout
    UID get_checkout_id() const { return current_head_; }

    // checks out the current checkout to be active (scratch)
    //
    // effectively, reset the scratch space
    void checkout()
    {
        // because this is a "reset", try to maintain useful state from the
        // scratch space - things like reset and scaling state, which the
        // user might expect to be maintained even if a crash happened

        const ModelStateCommit* c = try_get_commit_by_id(current_head_);

        if (c) {
            UiModelStatePair new_scratch{std::make_unique<OpenSim::Model>(*c->get_model())};
            copy_selected_and_hovered(scratch_, new_scratch);
            new_scratch.set_fixup_scale_factor(scratch_.get_fixup_scale_factor());
            scratch_ = std::move(new_scratch);
        }
    }

    // performs an undo, if possible
    //
    // effectively, checks out HEAD~1
    void undo()
    {
        const ModelStateCommit* c = try_get_commit_by_id(current_head_);

        if (not c) {
            return;
        }

        const ModelStateCommit* parent = try_get_commit_by_id(c->get_parent_id());

        if (not parent) {
            return;
        }

        // perform fixups to ensure the user experience is best:
        //
        // - user's selection state should be "sticky" between undo/redo
        // - user's scene scale factor should be "sticky" between undo/redo
        UiModelStatePair new_model{std::make_unique<OpenSim::Model>(*parent->get_model())};
        copy_selected_and_hovered(scratch_, new_model);
        new_model.set_fixup_scale_factor(scratch_.get_fixup_scale_factor());

        scratch_ = std::move(new_model);
        current_head_ = parent->get_id();
    }

    // performs a redo, if possible
    void redo()
    {
        int dist = distance(branch_head_, current_head_);

        if (dist < 1) {
            return;
        }

        const ModelStateCommit* c = nth_ancestor(branch_head_, dist - 1);

        if (not c) {
            return;
        }

        // perform fixups to ensure the user experience is best:
        //
        // - user's selection state should be "sticky" between undo/redo
        // - user's scene scale factor should be "sticky" between undo/redo
        UiModelStatePair new_model{std::make_unique<OpenSim::Model>(*c->get_model())};
        copy_selected_and_hovered(scratch_, new_model);
        new_model.set_fixup_scale_factor(scratch_.get_fixup_scale_factor());

        scratch_ = std::move(new_model);
        current_head_ = c->get_id();
    }

    std::filesystem::path get_filesystem_location() const
    {
        return std::filesystem::path{get_model().getInputFileName()};
    }

    void set_filesystem_location(const std::filesystem::path& p)
    {
        upd_model().setInputFileName(p.string());
    }

    UID get_filesystem_version() const { return maybe_commit_saved_to_disk_; }

    // mutable staging area that calling code can mutate
    UiModelStatePair scratch_;

    // where scratch will commit to (i.e. the parent of the scratch area)
    UID current_head_ = UID::empty();

    // head of the current branch (i.e. "main") - may be ahead of current branch (undo/redo)
    UID branch_head_ = UID::empty();

    // underlying storage for immutable commits
    std::unordered_map<UID, ModelStateCommit> commits_;

    // the timestamp of the on-disk data (needed to know when to trigger a reload)
    std::filesystem::file_time_type maybe_filesystem_timestamp_;

    // (maybe) the version of the model that was last saved to disk
    UID maybe_commit_saved_to_disk_ = UID::empty();

    // the `Environment` that this model is associated with
    std::shared_ptr<Environment> environment_ = std::make_shared<Environment>();
};

osc::UndoableModelStatePair::UndoableModelStatePair() :
    impl_{std::make_unique<Impl>()}
{}

osc::UndoableModelStatePair::UndoableModelStatePair(const OpenSim::Model& model) :
    impl_{std::make_unique<Impl>(model)}
{}

osc::UndoableModelStatePair::UndoableModelStatePair(std::unique_ptr<OpenSim::Model> model) :
    impl_{std::make_unique<Impl>(std::move(model))}
{}

osc::UndoableModelStatePair::UndoableModelStatePair(const std::filesystem::path& osim_path) :
    impl_{std::make_unique<Impl>(osim_path)}
{}

osc::UndoableModelStatePair::UndoableModelStatePair(const UndoableModelStatePair& src) :
    impl_{std::make_unique<Impl>(*src.impl_)}
{}

osc::UndoableModelStatePair::UndoableModelStatePair(UndoableModelStatePair&&) noexcept = default;

UndoableModelStatePair& osc::UndoableModelStatePair::operator=(const UndoableModelStatePair& src)
{
    if (&src != this) {
        std::unique_ptr<Impl> cpy = std::make_unique<Impl>(*src.impl_);
        std::swap(impl_, cpy);
    }

    return *this;
}

UndoableModelStatePair& osc::UndoableModelStatePair::operator=(UndoableModelStatePair&&) noexcept =
    default;
osc::UndoableModelStatePair::~UndoableModelStatePair() noexcept = default;

bool osc::UndoableModelStatePair::is_up_to_date_with_filesystem() const
{
    return impl_->is_up_to_date_with_filesystem();
}

std::filesystem::file_time_type osc::UndoableModelStatePair::get_last_filesystem_write_time() const
{
    return impl_->get_last_filesystem_write_time();
}

ModelStateCommit osc::UndoableModelStatePair::get_latest_commit() const
{
    return impl_->get_latest_commit();
}

bool osc::UndoableModelStatePair::can_undo() const
{
    return impl_->can_undo();
}

void osc::UndoableModelStatePair::do_undo()
{
    impl_->do_undo();
}

bool osc::UndoableModelStatePair::can_redo() const
{
    return impl_->can_redo();
}

void osc::UndoableModelStatePair::do_redo()
{
    impl_->do_redo();
}

void osc::UndoableModelStatePair::rollback()
{
    impl_->rollback();
}

bool osc::UndoableModelStatePair::try_checkout(const ModelStateCommit& commit)
{
    return impl_->try_checkout(commit);
}

void osc::UndoableModelStatePair::set_model(std::unique_ptr<OpenSim::Model> new_model)
{
    impl_->set_model(std::move(new_model));
}

void osc::UndoableModelStatePair::reset_model()
{
    impl_->reset_model();
}

void osc::UndoableModelStatePair::load_model(const std::filesystem::path& p)
{
    impl_->load_model(p);
}

void osc::UndoableModelStatePair::impl_set_model_version(UID version)
{
    impl_->set_model_version(version);
}

const OpenSim::Model& osc::UndoableModelStatePair::impl_get_model() const
{
    return impl_->get_model();
}

const SimTK::State& osc::UndoableModelStatePair::impl_get_state() const
{
    return impl_->get_state();
}

OpenSim::Model& osc::UndoableModelStatePair::impl_upd_model()
{
    return impl_->upd_model();
}

void osc::UndoableModelStatePair::impl_commit(std::string_view commit_message)
{
    impl_->commit(commit_message);
}

UID osc::UndoableModelStatePair::impl_get_model_version() const
{
    return impl_->get_model_version();
}

UID osc::UndoableModelStatePair::impl_get_state_version() const
{
    return impl_->get_state_version();
}

float osc::UndoableModelStatePair::impl_get_fixup_scale_factor() const
{
    return impl_->get_fixup_scale_factor();
}

void osc::UndoableModelStatePair::impl_set_fixup_scale_factor(float v)
{
    impl_->set_fixup_scale_factor(v);
}

const OpenSim::Component* osc::UndoableModelStatePair::impl_get_selected() const
{
    return impl_->get_selected();
}

void osc::UndoableModelStatePair::impl_set_selected(const OpenSim::Component* c)
{
    impl_->set_selected(c);
}

const OpenSim::Component* osc::UndoableModelStatePair::impl_get_hovered() const
{
    return impl_->get_hovered();
}

void osc::UndoableModelStatePair::impl_set_hovered(const OpenSim::Component* c)
{
    impl_->set_hovered(c);
}

std::shared_ptr<Environment> osc::UndoableModelStatePair::impl_upd_associated_environment() const
{
    return impl_->impl_upd_associated_environment();
}

void osc::UndoableModelStatePair::impl_set_up_to_date_with_filesystem(
    std::filesystem::file_time_type t)
{
    impl_->set_up_to_date_with_filesystem(t);
}
