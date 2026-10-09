#pragma once

#include <libopensimcreator/documents/mesh_importer/mi_class.h>
#include <libopensimcreator/documents/mesh_importer/mi_object.h>
#include <libopensimcreator/documents/mesh_importer/mi_variant_reference.h>

#include <memory>
#include <type_traits>

namespace osc
{
    // Curiously Recurring Template Pattern (CRTP) for MiObject
    //
    // automatically defines parts of the MiObject API using CRTP, so that
    // downstream classes don't have to repeat themselves
    template<typename T>
    class MiObjectCRTP : public MiObject {
    public:
        static const MiClass& Class()
        {
            static const MiClass s_Class = T::CreateClass();
            return s_Class;
        }

        std::unique_ptr<T> clone()
        {
            return std::unique_ptr<T>{static_cast<T*>(implClone().release())};
        }
    private:
        const MiClass& implGetClass() const final
        {
            static_assert(std::is_reference_v<decltype(T::Class())>);
            static_assert(std::is_same_v<decltype(T::Class()), const MiClass&>);
            return T::Class();
        }

        std::unique_ptr<MiObject> implClone() const final
        {
            static_assert(std::is_base_of_v<MiObject, T>);
            static_assert(std::is_final_v<T>);
            return std::make_unique<T>(static_cast<T const&>(*this));
        }

        MiVariantConstReference implToVariant() const final
        {
            static_assert(std::is_base_of_v<MiObject, T>);
            static_assert(std::is_final_v<T>);
            return static_cast<T const&>(*this);
        }

        MiVariantReference implToVariant() final
        {
            static_assert(std::is_base_of_v<MiObject, T>);
            static_assert(std::is_final_v<T>);
            return static_cast<T&>(*this);
        }
    };
}
