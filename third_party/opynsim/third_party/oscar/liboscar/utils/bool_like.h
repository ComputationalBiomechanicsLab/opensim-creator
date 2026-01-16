#pragma once

namespace osc
{
    // A type that directly wraps a `bool` (as in, it has the same object
    // representation as a `bool`).
    //
    // The main reason this exists is to provide a separate type from `bool`
    // for template overloading (i.e. you might need this whenever `std::vector<bool>`
    // is being a PITA).
    class BoolLike final {
    public:
        BoolLike() = default;
        BoolLike(bool value_) : value{value_} {}

        operator bool& () { return value; }
        operator const bool& () const { return value; }

    private:
        bool value;
    };

    inline bool* cast_to_bool_ptr(BoolLike* bool_like)
    {
        return reinterpret_cast<bool*>(bool_like);
    }

    inline const bool* cast_to_bool_ptr(const BoolLike* bool_like)
    {
        return reinterpret_cast<const bool*>(bool_like);
    }
}
