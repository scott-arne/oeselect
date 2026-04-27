/**
 * @file glob_match.h
 * @brief Minimal portable wildcard matcher for atom/residue name predicates.
 *
 * Supports only the two metacharacters the parser grammar accepts:
 *  - `*` matches zero or more characters.
 *  - `?` matches exactly one character.
 *
 * No character classes, no escapes — the grammar (src/Parser.cpp glob_char)
 * cannot produce them. This header is private to the library (src/ only)
 * and is not installed.
 */

#ifndef OESELECT_GLOB_MATCH_H
#define OESELECT_GLOB_MATCH_H

#include <string>

namespace OESel {

/**
 * @brief Match a string against a wildcard pattern.
 *
 * :param pattern: Pattern containing literal chars plus `*` and `?`.
 * :param text: String to test.
 * :returns: ``true`` if the pattern matches the entire text.
 */
inline bool match_glob(const std::string& pattern, const std::string& text) {
    const size_t np = pattern.size();
    const size_t nt = text.size();
    size_t pi = 0;
    size_t ti = 0;
    size_t star_pi = std::string::npos;  // position in pattern after last '*'
    size_t star_ti = 0;                  // position in text when we saw that '*'

    while (ti < nt) {
        if (pi < np && (pattern[pi] == '?' || pattern[pi] == text[ti])) {
            ++pi;
            ++ti;
        } else if (pi < np && pattern[pi] == '*') {
            star_pi = ++pi;
            star_ti = ti;
        } else if (star_pi != std::string::npos) {
            pi = star_pi;
            ti = ++star_ti;
        } else {
            return false;
        }
    }
    while (pi < np && pattern[pi] == '*') {
        ++pi;
    }
    return pi == np;
}

}  // namespace OESel

#endif  // OESELECT_GLOB_MATCH_H
