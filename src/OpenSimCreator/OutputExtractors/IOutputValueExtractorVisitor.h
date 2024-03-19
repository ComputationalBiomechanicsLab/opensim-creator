#pragma once

namespace osc { class IFloatOutputValueExtractor; }
namespace osc { class IStringOutputValueExtractor; }

namespace osc
{
    // a visitor (as in, visitor pattern) that is `accept`ed by `IOutputExtractor`
    // implementations, which enables runtime double-dispatch to a concrete output
    // value type
    class IOutputValueExtractorVisitor {
    protected:
        IOutputValueExtractorVisitor() = default;
        IOutputValueExtractorVisitor(IOutputValueExtractorVisitor const&) = default;
        IOutputValueExtractorVisitor(IOutputValueExtractorVisitor&&) noexcept = default;
        IOutputValueExtractorVisitor& operator=(IOutputValueExtractorVisitor const&) = default;
        IOutputValueExtractorVisitor& operator=(IOutputValueExtractorVisitor&&) noexcept = default;
    public:
        virtual ~IOutputValueExtractorVisitor() noexcept = default;

        virtual void operator()(IFloatOutputValueExtractor const&) = 0;
        virtual void operator()(IStringOutputValueExtractor const&) = 0;
    };
}
