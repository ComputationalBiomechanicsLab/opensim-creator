#pragma once

#include <OpenSimCreator/OutputExtractors/IOutputValueExtractorVisitor.h>

#include <concepts>
#include <utility>

namespace osc
{
    template<
        std::invocable<IFloatOutputValueExtractor const&> FloatCallback,
        std::invocable<IVec2OutputValueExtractor const&> Vec2Callback,
        std::invocable<IStringOutputValueExtractor const&> StringCallback
    >
    class LambdaOutputValueExtractorVisitor final : public IOutputValueExtractorVisitor {
    public:
        LambdaOutputValueExtractorVisitor(
            FloatCallback&& floatCallback_,
            Vec2Callback&& vec2Callback_,
            StringCallback&& stringCallback_) :

            m_FloatCallback{std::forward<FloatCallback>(floatCallback_)},
            m_Vec2Callback{std::forward<Vec2Callback>(vec2Callback_)},
            m_StringCallback{std::forward<StringCallback>(stringCallback_)}
        {}
    private:
        void operator()(IFloatOutputValueExtractor const& extractor) override { m_FloatCallback(extractor); }
        void operator()(IVec2OutputValueExtractor const& extractor) override { m_Vec2Callback(extractor); }
        void operator()(IStringOutputValueExtractor const& extractor) override { m_StringCallback(extractor); }

        FloatCallback m_FloatCallback;
        Vec2Callback m_Vec2Callback;
        StringCallback m_StringCallback;
    };
}
