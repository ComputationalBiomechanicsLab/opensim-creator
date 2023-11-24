#pragma once

#include <OpenSimCreator/Documents/ModelGraph/SceneEl.hpp>
#include <OpenSimCreator/Documents/ModelGraph/SceneElClass.hpp>
#include <OpenSimCreator/Documents/ModelGraph/SceneElVariant.hpp>

#include <memory>
#include <type_traits>

namespace osc
{
    // Curiously Recurring Template Pattern (CRTP) for SceneEl
    //
    // automatically defines parts of the SceneEl API using CRTP, so that
    // downstream classes don't have to repeat themselves
    template<class T>
    class SceneElCRTP : public SceneEl {
    public:
        static SceneElClass const& Class()
        {
            static SceneElClass const s_Class = T::CreateClass();
            return s_Class;
        }

        std::unique_ptr<T> clone()
        {
            return std::unique_ptr<T>{static_cast<T*>(implClone().release())};
        }
    private:
        SceneElClass const& implGetClass() const final
        {
            static_assert(std::is_reference_v<decltype(T::Class())>);
            static_assert(std::is_same_v<decltype(T::Class()), SceneElClass const&>);
            return T::Class();
        }

        std::unique_ptr<SceneEl> implClone() const final
        {
            static_assert(std::is_base_of_v<SceneEl, T>);
            static_assert(std::is_final_v<T>);
            return std::make_unique<T>(static_cast<T const&>(*this));
        }

        ConstSceneElVariant implToVariant() const final
        {
            static_assert(std::is_base_of_v<SceneEl, T>);
            static_assert(std::is_final_v<T>);
            return static_cast<T const&>(*this);
        }

        SceneElVariant implToVariant() final
        {
            static_assert(std::is_base_of_v<SceneEl, T>);
            static_assert(std::is_final_v<T>);
            return static_cast<T&>(*this);
        }
    };
}
