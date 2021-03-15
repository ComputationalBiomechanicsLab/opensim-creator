#pragma once

#include <cassert>
#include <optional>
#include <utility>

namespace osmv {
    template<typename T>
    class Indirect_ptr {
    public:
        class Guard {
            Indirect_ptr* holder;
            T* ptr;

        public:
            constexpr Guard(Indirect_ptr* _holder, T* _ptr) noexcept : holder{_holder}, ptr{_ptr} {
            }

            Guard(Guard const&) = delete;
            Guard(Guard&&) = delete;
            Guard& operator=(Guard const&) = delete;
            Guard& operator=(Guard&&) = delete;

            ~Guard() noexcept {
                holder->on_end_modify();
            }

            [[nodiscard]] constexpr T& operator*() const noexcept {
                return *ptr;
            }

            [[nodiscard]] constexpr T* operator->() noexcept {
                return ptr;
            }
        };

        virtual ~Indirect_ptr() noexcept = default;

        [[nodiscard]] T const* get() {
            return impl_get();
        }

        [[nodiscard]] T const* operator->() {
            return impl_get();
        }

        [[nodiscard]] T const& operator*() {
            return *impl_get();
        }

        operator T const*() {
            return impl_get();
        }

        operator bool() {
            return impl_get() != nullptr;
        }

        [[nodiscard]] friend bool operator==(Indirect_ptr<T>& a, T const* b) {
            return a.impl_get() == b;
        }

        [[nodiscard]] friend bool operator==(T const* a, Indirect_ptr<T>& b) {
            return a == b.impl_get();
        }

        [[nodiscard]] friend bool operator!=(Indirect_ptr<T>& a, T const* b) {
            return not(a == b);
        }

        [[nodiscard]] friend bool operator!=(T const* a, Indirect_ptr<T>& b) {
            return not(a == b);
        }

        void reset(T* ptr) {
            impl_set(ptr);
        }

        void reset(T const* ptr) {
            impl_set(const_cast<T*>(ptr));
        }

        void reset() {
            impl_set(nullptr);
        }

        [[nodiscard]] Guard modify() {
            T* ptr = impl_upd();
            on_begin_modify();
            return Guard{this, ptr};
        }

        template<typename Modifier>
        void apply_modification(Modifier f) {
            auto guard = modify();
            f(*guard);
        }

        T* UNSAFE_upd() {
            return impl_upd();
        }

        void UNSAFE_on_begin_modify() {
            on_begin_modify();
        }

        void UNSAFE_on_end_modify() {
            on_end_modify();
        }

    private:
        virtual T* impl_upd() = 0;

        virtual T const* impl_get() {
            return impl_upd();
        }

        virtual void impl_set(T*) = 0;

        virtual void on_begin_modify() {
        }

        virtual void on_end_modify() noexcept {
        }
    };

    template<typename TBase, typename TDerived>
    class Downcasted_indirect_ptr final : public Indirect_ptr<TDerived> {
    private:
        Indirect_ptr<TBase>& proxy;

        // this factory function is the only way to get access to one of these
        template<typename TDerived_, typename TBase_>
        friend std::optional<Downcasted_indirect_ptr<TBase_, TDerived_>> try_downcast(Indirect_ptr<TBase_>&);

        Downcasted_indirect_ptr(Indirect_ptr<TBase>& _proxy) : proxy{_proxy} {
            assert(dynamic_cast<TDerived const*>(proxy.get()));
        }

        TDerived* impl_upd() override {
            TBase* p = proxy.UNSAFE_upd();
            assert(dynamic_cast<TDerived*>(p));
            return static_cast<TDerived*>(p);
        }

        TDerived const* impl_get() override {
            TBase const* p = proxy.get();
            assert(dynamic_cast<TDerived const*>(p));
            return static_cast<TDerived const*>(p);
        }

        void impl_set(TDerived* p) override {
            proxy.reset(p);
        }

        void on_begin_modify() override {
            proxy.UNSAFE_on_begin_modify();
        }

        void on_end_modify() noexcept override {
            proxy.UNSAFE_on_end_modify();
        }
    };

    template<typename TDerived, typename TBase>
    std::optional<Downcasted_indirect_ptr<TBase, TDerived>> try_downcast(Indirect_ptr<TBase>& p) {
        if (dynamic_cast<TDerived const*>(p.get())) {
            return Downcasted_indirect_ptr<TBase, TDerived>{p};
        } else {
            return std::nullopt;
        }
    }

    template<typename T>
    class Trivial_indirect_ptr final : public Indirect_ptr<T> {
        T** ptr2ptr;

    public:
        constexpr Trivial_indirect_ptr(T** _ptr2ptr) noexcept : ptr2ptr{_ptr2ptr} {
            assert(ptr2ptr != nullptr);
        }

    private:
        T* impl_upd() override {
            return *ptr2ptr;
        }

        void impl_set(T* p) override {
            *ptr2ptr = p;
        }
    };
}
