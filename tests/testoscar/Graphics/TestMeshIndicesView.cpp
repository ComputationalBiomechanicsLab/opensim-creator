#include <oscar/Graphics/MeshIndicesView.h>

#include <gtest/gtest.h>

#include <concepts>
#include <ranges>

using namespace osc;

TEST(MeshIndicesView, IsARange)
{
    // required for it to be used in generic C++20 range algorithms

    static_assert(std::movable<typename MeshIndicesView::iterator>);
    static_assert(std::weakly_incrementable<typename MeshIndicesView::iterator>);
    static_assert(std::input_or_output_iterator<typename MeshIndicesView::iterator>);
    static_assert(std::ranges::range<MeshIndicesView>);
}
