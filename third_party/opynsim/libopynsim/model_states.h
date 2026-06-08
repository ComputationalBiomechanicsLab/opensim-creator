#pragma once

#include <libopynsim/model_state.h>

#include <concepts>
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

        /// Returns a reference to the element at specified location `pos`, with bounds checking.
        reference at(size_t pos) { return *states_.at(pos); }

        /// Returns a reference to the element at specified location `pos`, with bounds checking.
        const_reference at(size_t pos) const { return *states_.at(pos); }

        /// Increases the capacity of the container to a value that's greater or equal to `new_cap`.
        void reserve(size_type new_cap) { states_.reserve(new_cap); }

        /// Appends a new `ModelState` to the end of the sequence. The `ModelState` is
        /// constructed in-place with the provided arguments.
        template<typename... Args>
        requires std::constructible_from<ModelState, Args&&...>
        reference emplace_back(Args&&... args)
        {
            return *states_.emplace_back(std::make_shared<ModelState>(std::forward<Args>(args)...));
        }

        /// Appends a shared handle of `ModelState` to the end of the sequence.
        void handle_push_back(shared_handle handle) { states_.push_back(std::move(handle)); }

        /// Returns a shared handle to the element at specified location `pos`, with bounds checking.
        shared_handle handle_at(size_t pos) { return states_.at(pos); }

        /// Converts this `ModelStates` into a `std::vector` its `shared_handle`s.
        std::vector<shared_handle> to_handle_list() const { return states_; }

    private:
        auto view() const { return states_ | std::views::transform([](const auto& handle) -> const_reference { return *handle; }); }
    public:
        auto begin() const { return view().begin(); }
        auto end() const { return view().end(); }

    private:
        std::vector<shared_handle> states_;
    };
}
