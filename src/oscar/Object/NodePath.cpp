#include "NodePath.hpp"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <string_view>

using namespace osc;

namespace
{
    constexpr std::string_view c_NewInvalidChars{"\\*+ \t\n"};
    constexpr std::string::value_type c_NUL = {};

    // Returns a normalized form of `path`. A normalized path string is
    // guaranteed to:
    //
    // - Not contain any *internal* or *trailing* relative elements (e.g.
    //   'a/../b').
    //
    //     - It may *start* with relative elements (e.g. '../a/b'), but only
    //       if the path is non-absolute (e.g. "/../a/b" is invalid)
    //
    // - Not contain any invalid characters (e.g. '\\', '*')
    //
    // - Not contain any repeated separators (e.g. 'a///b' --> 'a/b')
    //
    // Any attempt to step above the root of the expression with '..' will
    // result in an exception being thrown (e.g. "a/../.." will throw).
    //
    // This method is useful for path traversal and path manipulation
    // methods, because the above ensures that (e.g.) paths can be
    // concatenated and split into individual elements using basic
    // string manipulation techniques.
    std::string normalize(std::string path)
    {
        using Iter = typename std::string::iterator;
        using ConstIter = typename std::string::const_iterator;
        using Value = typename std::string::value_type;

        // note: this implementation is fairly low-level and involves mutating
        //       `path` quite a bit. The test suite is heavily relied on for
        //       developing this kind of tricky code.
        //
        //       the reason it's done this way is because profiling shown that
        //       `ComponentPath` was accounting for ~6-10 % of OpenSim's CPU
        //       usage for component-heavy sims. The reason this was so high
        //       was because `ComponentPath` used a simpler algorithm that
        //       split the path into a std::vector.
        //
        //       usually, that alg. wouldn't be a problem, but path
        //       normalization can happen millions of times during a simulation,
        //       all those vector allocations can thrash the allocator and
        //       increase L1 misses.

        // assert that `path` contains no invalid chars
        if (path.find_first_of(c_NewInvalidChars) != std::string::npos)
        {
            throw std::runtime_error{path + ": The supplied path contains invalid characters."};
        }

        // pathEnd is guaranteed to be a NUL terminator since C++11
        Iter const pathBegin = path.begin();
        Iter pathEnd = path.end();

        // helper: shift n chars starting at newStart+n such that, after,
        // newStart..end is equal to what newStart+n..end was before.
        auto const shift = [&pathEnd](Iter newStart, size_t n)
        {
            std::copy(newStart + n, pathEnd, newStart);
            pathEnd -= n;
        };

        // helper: grab 3 lookahead chars, using NUL as a senteniel to
        // indicate "past the end of the content".
        //
        // - The maximum lookahead is 3 characters because the parsing
        //   code below needs to be able to detect the upcoming input
        //   pattern "..[/\0]"
        struct Lookahead { Value a, b, c; };
        auto getLookahead = [](ConstIter start, ConstIter end)
        {
            return Lookahead
            {
                start < end - 0 ? start[0] : c_NUL,
                start < end - 1 ? start[1] : c_NUL,
                start < end - 2 ? start[2] : c_NUL,
            };
        };

        // remove duplicate adjacent separators
        for (Iter it = pathBegin; it != pathEnd; ++it)
        {
            Lookahead l = getLookahead(it, pathEnd);
            if (l.a == NodePath::separator && l.b == NodePath::separator)
            {
                shift(it--, 1);
            }
        }

        bool isAbsolute = *pathBegin == NodePath::separator;
        Iter cursor = isAbsolute ? pathBegin + 1 : pathBegin;

        // skip/dereference relative elements *at the start of a path*
        {
            Lookahead l = getLookahead(cursor, pathEnd);
            while (l.a == '.') {
                switch (l.b) {
                case NodePath::separator:
                    shift(cursor, 2);
                    break;
                case c_NUL:
                    shift(cursor, 1);
                    break;
                case '.': {
                    if (l.c == NodePath::separator|| l.c == c_NUL) {
                        // starts with '..' element: only allowed if the path
                        // is relative
                        if (isAbsolute)
                        {
                            throw std::runtime_error{path + ": is an invalid path: it is absolute, but starts with relative elements."};
                        }

                        // if not absolute, then make sure `contentStart` skips
                        // past these elements because the alg can't reduce
                        // them down
                        if (l.c == NodePath::separator) {
                            cursor += 3;
                        } else {
                            cursor += 2;
                        }
                    } else {
                        // normal element that starts with '..'
                        ++cursor;
                    }
                    break;
                }
                default:
                    // normal element that starts with '.'
                    ++cursor;
                    break;
                }

                l = getLookahead(cursor, pathEnd);
            }
        }

        Iter contentStart = cursor;

        // invariants:
        //
        // - the root path element (if any) has been skipped
        // - `contentStart` points to the start of the non-relative content of
        //   the supplied path string
        // - `path` contains no duplicate adjacent separators
        // - `[0..offset]` is normalized path string, but may contain a
        //   trailing slash
        // - `[contentStart..offset] is the normalized *content* of the path
        //   string

        while (cursor < pathEnd) {
            Lookahead l = getLookahead(cursor, pathEnd);

            if (l.a == '.' && (l.b == c_NUL || l.b == NodePath::separator)) {
                // handle '.' (if found)
                size_t charsInCurEl = l.b == NodePath::separator ? 2 : 1;
                shift(cursor, charsInCurEl);

            } else if (l.a == '.' && l.b == '.' && (l.c == c_NUL || l.c == NodePath::separator)) {
                // handle '..' (if found)

                if (cursor == contentStart)
                {
                    throw std::runtime_error{path + ": cannot handle '..' element in a path string: dereferencing this would hop above the root of the path."};
                }

                // search backwards for previous separator
                Iter prevSeparator = cursor - 2;
                while (prevSeparator > contentStart && *prevSeparator != NodePath::separator)
                {
                    --prevSeparator;
                }

                Iter prevStart = prevSeparator <= contentStart ? contentStart : prevSeparator + 1;
                size_t charsInCurEl = (l.c == NodePath::separator) ? 3 : 2;
                size_t charsInPrevEl = cursor - prevStart;

                cursor = prevStart;
                shift(cursor, charsInPrevEl + charsInCurEl);

            } else {
                // non-relative element: skip past the next separator or end
                cursor = std::find(cursor, pathEnd, NodePath::separator) + 1;
            }
        }

        // edge case:
        // - There was a trailing slash in the input and, post reduction, the output
        //   string is only a slash. However, the input path wasnt initially an
        //   absolute path, so the output should be "", not "/"
        {
            Iter beg = isAbsolute ? pathBegin + 1 : pathBegin;
            if (pathEnd - beg > 0 && pathEnd[-1] == NodePath::separator)
            {
                --pathEnd;
            }
        }

        // resize output to only contain the normalized range
        path.resize(pathEnd - pathBegin);

        return path;
    }
}

osc::NodePath::NodePath(std::string_view p) :
    m_ParsedPath{normalize(std::string{p})}
{
}
