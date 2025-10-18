#pragma once

#include <cstddef>
#include <optional>
#include <stdexcept>

namespace osc::detail
{
    class MaybeIndex final {
    public:
        constexpr MaybeIndex() = default;
        explicit constexpr MaybeIndex(std::optional<size_t> tagged)
        {
            if (tagged) {
                if (tagged.value() == c_senteniel_index_value) {
                    throw std::invalid_argument{"the provided index value is out of range"};
                }
                value_ = *tagged;
            }
        }

        friend bool operator==(MaybeIndex, MaybeIndex) = default;
        friend bool operator==(MaybeIndex lhs, std::optional<size_t> rhs) { return static_cast<std::optional<size_t>>(lhs) == rhs; }

        explicit operator bool () const { return value_ != c_senteniel_index_value; }
        explicit operator std::optional<size_t> () const { return value_ != c_senteniel_index_value ? std::optional<size_t>{value_} : std::nullopt; }
        size_t operator*() const { return value_; }
    private:
        static constexpr size_t c_senteniel_index_value = std::numeric_limits<size_t>::max();
        size_t value_ = c_senteniel_index_value;
    };
}
