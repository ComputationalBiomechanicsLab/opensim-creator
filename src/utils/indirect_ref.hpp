#pragma once

#include <utility>

namespace osmv {
    template<typename T>
    class Indirect_ref {
    public:
        class Guard {
            Indirect_ref* holder;
            T& ref;

        public:
            constexpr Guard(Indirect_ref* _holder, T& _ref) noexcept : holder{_holder}, ref{_ref} {
            }

            Guard(Guard const&) = delete;
            Guard(Guard&&) = delete;
            Guard& operator=(Guard const&) = delete;
            Guard& operator=(Guard&&) = delete;

            ~Guard() noexcept {
                holder->on_end_modify();
            }

            constexpr operator T&() const noexcept {
                return ref;
            }

            [[nodiscard]] constexpr T& get() const noexcept {
                return ref;
            }

            [[nodiscard]] constexpr T* operator->() const noexcept {
                return &ref;
            }
        };

        virtual ~Indirect_ref() noexcept = default;

        [[nodiscard]] T const& get() {
            return impl_get();
        }

        [[nodiscard]] Guard modify() {
            T& ref = impl_upd();
            on_begin_modify();
            return Guard{this, ref};
        }

        template<typename Modifier>
        void apply_modification(Modifier f) {
            auto guard = modify();
            f(guard.get());
        }

        // UNSAFE because it violates the contract of calling the virtual `on_begin_modify` before
        // editing and `on_end_modify` after editing. This is a backdoor method for when the caller
        // knows what they're doing
        T& UNSAFE_upd() {
            return impl_upd();
        }

        // UNSAFE because it is manual. *Most* callers should use the non-UNSAFE API. However,
        // *some* callers will want more manual control - this method is for the latter group
        void UNSAFE_on_begin_modify() {
            on_begin_modify();
        }

        // UNSAFE because it is manual. *Most* callers should use the non-UNSAFE API. However,
        // *some* callers will want more manual control - this method is for the latter group
        void UNSAFE_on_end_modify() {
            on_end_modify();
        }

    private:
        // downstream implementations should provide these

        virtual T& impl_upd() = 0;

        virtual T const& impl_get() {
            return impl_upd();
        }

        virtual void on_begin_modify() {
        }

        virtual void on_end_modify() noexcept {
        }
    };

    template<typename T>
    class Trivial_indirect_ref final : public Indirect_ref<T> {
        T& ref;

    public:
        // implicit conversion from a reference w/o update semantics
        constexpr Trivial_indirect_ref(T& _ref) noexcept : ref{_ref} {
        }

    private:
        T& impl_upd() override {
            return ref;
        }
    };

    template<typename T, typename Getter, typename ModificationCallback>
    class Lambda_indirect_ref final : public Indirect_ref<T> {
        Getter getter;
        ModificationCallback _on_end_modify;

    public:
        Lambda_indirect_ref(Getter _getter, ModificationCallback cb) :
            getter{std::move(_getter)},
            _on_end_modify{std::move(cb)} {
        }

    private:
        T& impl_upd() {
            return getter();
        }

        void on_end_modify() noexcept override {
            on_end_modify();
        }
    };
}
