#include "NodePath.h"

#include <oscar/Utils/Algorithms.h>

#include <algorithm>
#include <stdexcept>
#include <string>
#include <string_view>

using namespace osc;

namespace
{
    constexpr std::string_view c_invalid_chars = "\\*+ \t\n";
    constexpr std::string::value_type c_nul = {};

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
        if (path.find_first_of(c_invalid_chars) != std::string::npos) {
            throw std::runtime_error{path + ": The supplied path contains invalid characters."};
        }

        // path_end is guaranteed to be a NUL terminator since C++11
        const Iter path_begin = path.begin();
        Iter path_end = path.end();

        // helper: shift n chars starting at new_start+n such that, after,
        // new_start..end is equal to what new_start+n..end was before.
        const auto shift = [&path_end](Iter new_start, size_t n)
        {
            copy(new_start + n, path_end, new_start);
            path_end -= n;
        };

        // helper: grab 3 lookahead chars, using NUL as a senteniel to
        // indicate "past the end of the content".
        //
        // - The maximum lookahead is 3 characters because the parsing
        //   code below needs to be able to detect the upcoming input
        //   pattern "..[/\0]"
        struct Lookahead { Value a, b, c; };
        const auto get_lookahead = [](ConstIter start, ConstIter end)
        {
            return Lookahead{
                start < end - 0 ? start[0] : c_nul,
                start < end - 1 ? start[1] : c_nul,
                start < end - 2 ? start[2] : c_nul,
            };
        };

        // remove duplicate adjacent separators
        for (Iter it = path_begin; it != path_end; ++it) {
            const Lookahead l = get_lookahead(it, path_end);
            if (l.a == NodePath::separator and l.b == NodePath::separator) {
                shift(it--, 1);
            }
        }

        const bool is_absolute = *path_begin == NodePath::separator;
        Iter cursor = is_absolute ? path_begin + 1 : path_begin;

        // skip/dereference relative elements *at the start of a path*
        {
            Lookahead l = get_lookahead(cursor, path_end);
            while (l.a == '.') {
                switch (l.b) {
                case NodePath::separator:
                    shift(cursor, 2);
                    break;
                case c_nul:
                    shift(cursor, 1);
                    break;
                case '.': {
                    if (l.c == NodePath::separator or l.c == c_nul) {
                        // starts with '..' element: only allowed if the path
                        // is relative
                        if (is_absolute) {
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

                l = get_lookahead(cursor, path_end);
            }
        }

        const Iter content_start = cursor;

        // invariants:
        //
        // - the root path element (if any) has been skipped
        // - `contentStart` points to the start of the non-relative content of
        //   the supplied path string
        // - `path` contains no duplicate adjacent separators
        // - `[0..offset]` is normalized path string, but may contain a
        //   trailing slash
        // - `[content_start..offset] is the normalized *content* of the path
        //   string

        while (cursor < path_end) {
            const Lookahead l = get_lookahead(cursor, path_end);

            if (l.a == '.' and (l.b == c_nul or l.b == NodePath::separator)) {
                // handle '.' (if found)
                size_t num_chars_in_current_el = l.b == NodePath::separator ? 2 : 1;
                shift(cursor, num_chars_in_current_el);

            } else if (l.a == '.' and l.b == '.' and (l.c == c_nul or l.c == NodePath::separator)) {
                // handle '..' (if found)

                if (cursor == content_start) {
                    throw std::runtime_error{path + ": cannot handle '..' element in a path string: dereferencing this would hop above the root of the path."};
                }

                // search backwards for previous separator
                Iter prev_separator = cursor - 2;
                while (prev_separator > content_start and *prev_separator != NodePath::separator) {
                    --prev_separator;
                }

                const Iter prev_start = prev_separator <= content_start ? content_start : prev_separator + 1;
                const size_t num_chars_in_current_el = (l.c == NodePath::separator) ? 3 : 2;
                const size_t num_chars_in_previous_el = cursor - prev_start;

                cursor = prev_start;
                shift(cursor, num_chars_in_previous_el + num_chars_in_current_el);

            } else {
                // non-relative element: skip past the next separator or end
                cursor = find(cursor, path_end, NodePath::separator) + 1;
            }
        }

        // edge case:
        // - There was a trailing slash in the input and, post reduction, the output
        //   string is only a slash. However, the input path wasnt initially an
        //   absolute path, so the output should be "", not "/"
        {
            const Iter beg = is_absolute ? path_begin + 1 : path_begin;
            if (path_end - beg > 0 and path_end[-1] == NodePath::separator) {
                --path_end;
            }
        }

        // resize output to only contain the normalized range
        path.resize(path_end - path_begin);

        return path;
    }
}

osc::NodePath::NodePath(std::string_view p) :
    parsed_path_{normalize(std::string{p})}
{}
