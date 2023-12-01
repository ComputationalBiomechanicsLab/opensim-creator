#pragma once

#include <OpenSimCreator/Documents/MeshImporter/MIClass.hpp>
#include <OpenSimCreator/Documents/MeshImporter/MIObject.hpp>
#include <OpenSimCreator/Documents/MeshImporter/MIVariant.hpp>

#include <memory>
#include <type_traits>

namespace osc::mi
{
    // Curiously Recurring Template Pattern (CRTP) for MIObject
    //
    // automatically defines parts of the MIObject API using CRTP, so that
    // downstream classes don't have to repeat themselves
    template<class T>
    class MIObjectCRTP : public MIObject {
    public:
        static MIClass const& Class()
        {
            static MIClass const s_Class = T::CreateClass();
            return s_Class;
        }

        std::unique_ptr<T> clone()
        {
            return std::unique_ptr<T>{static_cast<T*>(implClone().release())};
        }
    private:
        MIClass const& implGetClass() const final
        {
            static_assert(std::is_reference_v<decltype(T::Class())>);
            static_assert(std::is_same_v<decltype(T::Class()), MIClass const&>);
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
