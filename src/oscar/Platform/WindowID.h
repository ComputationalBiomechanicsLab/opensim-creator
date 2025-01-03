#pragma once

namespace osc { class App; }

namespace osc
{
    class WindowID final {
    public:
        // Default constructs a "null" `WindowID`.
        WindowID() = default;

        // Constructs a `WindowID` from a type-erased void pointer. This is mostly
        // used when `WindowID`s are stored by third-party libraries.
        explicit WindowID(void* handle) : handle_{handle} {}

        friend bool operator==(const WindowID&, const WindowID&) = default;

        // Converts a `WindowID` to a boolean, which can be used to check for
        // its nullstate.
        explicit operator bool () const { return handle_ != nullptr; }

        // Converts a `WindowID` to a type-erased void pointer. This is mostly used
        // when `WindowID`s are stored by third-party libraries.
        explicit operator void* () const { return handle_; }

    private:
        void* handle_ = nullptr;
    };
}
