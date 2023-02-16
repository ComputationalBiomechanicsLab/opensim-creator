#pragma once

#include "src/Utils/Assertions.hpp"

#include <nonstd/span.hpp>

#include <cstdint>
#include <cstddef>

namespace osc
{
    // a span-like view over mesh indices
    //
    // for perf reasons, runtime mesh indices can be stored in either a 16-bit or 32-bit format
    // the mesh class exposes this fact by returning this view class, which must be checked at
    // runtime by calling code

    class MeshIndicesView final {
    private:
        union U32PtrOrU16Ptr {
            uint16_t const* u16;
            uint32_t const* u32;

            U32PtrOrU16Ptr() : u16{nullptr} {}
            U32PtrOrU16Ptr(uint16_t const* ptr) : u16{ptr} {}
            U32PtrOrU16Ptr(uint32_t const* ptr) : u32{ptr} {}
        };
    public:
        class Iterator final {
        public:
            Iterator(U32PtrOrU16Ptr ptr, bool isU32) noexcept : m_Ptr{ptr}, m_IsU32{isU32} {}

            uint32_t operator*() const noexcept
            {
                return m_IsU32 ? *m_Ptr.u32 : static_cast<uint32_t>(*m_Ptr.u16);
            }
            bool operator!=(Iterator const& other) const noexcept
            {
                return m_Ptr.u16 != other.m_Ptr.u16 || m_IsU32 != other.m_IsU32;
            }
            Iterator& operator++() noexcept
            {
                if (m_IsU32) { ++m_Ptr.u32; } else { ++m_Ptr.u16; }
                return *this;
            }
        private:
            U32PtrOrU16Ptr m_Ptr;
            bool m_IsU32;
        };

        MeshIndicesView() :
            m_Ptr{},
            m_Size{0},
            m_IsU32{false}
        {
        }

        MeshIndicesView(uint16_t const* ptr, size_t size) :
            m_Ptr{ptr},
            m_Size{size},
            m_IsU32{false}
        {
        }

        MeshIndicesView(nonstd::span<uint16_t const> span) :
            m_Ptr{span.data()},
            m_Size{span.size()},
            m_IsU32{false}
        {
        }

        MeshIndicesView(uint32_t const* ptr, size_t size) :
            m_Ptr{ptr},
            m_Size{size},
            m_IsU32{true}
        {
        }

        MeshIndicesView(nonstd::span<uint32_t const> span) :
            m_Ptr{span.data()},
            m_Size{span.size()},
            m_IsU32{true}
        {
        }

        bool isU16() const
        {
            return !m_IsU32;
        }

        bool isU32() const
        {
            return m_IsU32;
        }

        size_t size() const
        {
            return m_Size;
        }

        nonstd::span<uint16_t const> toU16Span() const
        {
            OSC_ASSERT(!m_IsU32);
            return {m_Ptr.u16, m_Size};
        }

        nonstd::span<uint32_t const> toU32Span() const
        {
            OSC_ASSERT(m_IsU32);
            return {m_Ptr.u32, m_Size};
        }

        uint32_t operator[](size_t i) const
        {
            return !m_IsU32 ? static_cast<uint32_t>(m_Ptr.u16[i]) : m_Ptr.u32[i];
        }

        Iterator begin() const noexcept
        {
            return Iterator{m_Ptr, m_IsU32};
        }

        Iterator end() const noexcept
        {
            return Iterator{m_IsU32 ? U32PtrOrU16Ptr{m_Ptr.u32 + m_Size} : U32PtrOrU16Ptr{m_Ptr.u16 + m_Size}, m_IsU32};
        }

    private:
        U32PtrOrU16Ptr m_Ptr;
        size_t m_Size;
        bool m_IsU32;
    };
}