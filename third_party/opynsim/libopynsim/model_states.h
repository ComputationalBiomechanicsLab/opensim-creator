#pragma once

#include <libopynsim/model_state.h>

#include <cstddef>
#include <memory>
#include <ranges>
#include <utility>
#include <vector>

namespace opyn
{
    /// Represents a sequence of `ModelState`s stored in shared handles.
    class ModelStates final {
    public:
        using value_type = ModelState;
        using size_type = size_t;
        using reference = value_type&;
        using const_reference = const value_type&;
        using shared_handle = std::shared_ptr<value_type>;

        /// Constructs an empty sequence of states.
        ModelStates() = default;

        /// Returns the number of elements in the container.
        size_t size() const { return states_.size(); }

        /// Appends a shared handle of `ModelState` to the end of the sequence.
        void handle_push_back(shared_handle handle) { states_.push_back(std::move(handle)); }

        /// Returns a shared handle to the element at specified location `pos`, with bounds checking.
        shared_handle handle_at(size_t pos) { return states_.at(pos); }

    private:
        auto view() const { return states_ | std::views::transform([](const auto& handle) -> const_reference { return *handle; }); }
    public:
        auto begin() const { return view().begin(); }
        auto end() const { return view().end(); }

    private:
        std::vector<shared_handle> states_;
    };
}
