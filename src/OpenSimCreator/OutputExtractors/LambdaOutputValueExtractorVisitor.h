#pragma once

#include <OpenSimCreator/OutputExtractors/IOutputValueExtractorVisitor.h>

#include <concepts>
#include <utility>

namespace osc
{
    template<
        std::invocable<IFloatOutputValueExtractor const&> FloatCallback,
        std::invocable<IStringOutputValueExtractor const&> StringCallback
    >
    class LambdaOutputValueExtractorVisitor final : public IOutputValueExtractorVisitor {
    public:
        LambdaOutputValueExtractorVisitor(
            FloatCallback&& floatCallback_,
            StringCallback&& stringCallback_) :

            m_FloatCallback{std::forward<FloatCallback>(floatCallback_)},
            m_StringCallback{std::forward<StringCallback>(stringCallback_)}
        {}
    private:
        void operator()(IFloatOutputValueExtractor const& extractor) override { m_FloatCallback(extractor); }
        void operator()(IStringOutputValueExtractor const& extractor) override { m_StringCallback(extractor); }

        FloatCallback m_FloatCallback;
        StringCallback m_StringCallback;
    };
}
