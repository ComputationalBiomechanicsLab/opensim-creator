#pragma once

#include <OpenSimCreator/Documents/ModelWarper/ICloneable.h>
#include <OpenSimCreator/Documents/ModelWarper/IDetailListable.h>
#include <OpenSimCreator/Documents/ModelWarper/IPointWarper.h>
#include <OpenSimCreator/Documents/ModelWarper/IValidateable.h>

#include <oscar/Maths/Vec3.h>

#include <memory>
#include <span>

namespace osc::mow { class Document; }

namespace osc::mow
{
    class IMeshWarp :
        public ICloneable<IMeshWarp>,
        public IDetailListable,
        public IValidateable {
    protected:
        IMeshWarp() = default;
        IMeshWarp(IMeshWarp const&) = default;
        IMeshWarp(IMeshWarp&&) noexcept = default;
        IMeshWarp& operator=(IMeshWarp const&) = default;
        IMeshWarp& operator=(IMeshWarp&&) noexcept = default;
    public:
        virtual ~IMeshWarp() = default;

        std::unique_ptr<IPointWarper> compileWarper(Document const& document) const { return implCompileWarper(document); }
    private:
        virtual std::unique_ptr<IPointWarper> implCompileWarper(Document const&) const = 0;
    };
}
