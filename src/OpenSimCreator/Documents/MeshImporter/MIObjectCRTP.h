#pragma once

#include <OpenSimCreator/Documents/MeshImporter/MIClass.h>
#include <OpenSimCreator/Documents/MeshImporter/MIObject.h>
#include <OpenSimCreator/Documents/MeshImporter/MIVariant.h>

#include <memory>
#include <type_traits>

namespace osc::mi
{
    // Curiously Recurring Template Pattern (CRTP) for MIObject
    //
    // automatically defines parts of the MIObject API using CRTP, so that
    // downstream classes don't have to repeat themselves
    template<typename T>
    class MIObjectCRTP : public MIObject {
    public:
        static const MIClass& Class()
        {
            static const MIClass s_Class = T::CreateClass();
            return s_Class;
        }

        std::unique_ptr<T> clone()
        {
            return std::unique_ptr<T>{static_cast<T*>(implClone().release())};
        }
    private:
        const MIClass& implGetClass() const final
        {
            static_assert(std::is_reference_v<decltype(T::Class())>);
            static_assert(std::is_same_v<decltype(T::Class()), const MIClass&>);
            return T::Class();
        }

        std::unique_ptr<MIObject> implClone() const final
        {
            static_assert(std::is_base_of_v<MIObject, T>);
            static_assert(std::is_final_v<T>);
            return std::make_unique<T>(static_cast<T const&>(*this));
        }

        ConstSceneElVariant implToVariant() const final
        {
            static_assert(std::is_base_of_v<MIObject, T>);
            static_assert(std::is_final_v<T>);
            return static_cast<T const&>(*this);
        }

        SceneElVariant implToVariant() final
        {
            static_assert(std::is_base_of_v<MIObject, T>);
            static_assert(std::is_final_v<T>);
            return static_cast<T&>(*this);
        }
    };
}
