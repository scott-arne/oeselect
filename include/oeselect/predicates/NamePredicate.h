/**
 * @file NamePredicate.h
 * @brief Atom name matching predicate.
 *
 * Matches atoms by their PDB atom name with optional glob-style wildcards.
 */

#ifndef OESELECT_PREDICATES_NAME_PREDICATE_H
#define OESELECT_PREDICATES_NAME_PREDICATE_H

#include "oeselect/Predicate.h"
#include <string>

namespace OESel {

/**
 * @brief Matches atoms by name with optional glob patterns.
 *
 * Supports exact matching or glob-style wildcards:
 * - `*` matches zero or more characters
 * - `?` matches exactly one character
 *
 * @code
 * // Exact match
 * NamePredicate("CA")   // Matches alpha carbons
 *
 * // Wildcard patterns
 * NamePredicate("C*")   // Matches CA, CB, CG, etc.
 * NamePredicate("?G")   // Matches CG, OG, etc.
 * @endcode
 */
class NamePredicate : public Predicate {
public:
    /**
     * @brief Construct name predicate.
     * @param pattern Atom name or glob pattern to match.
     */
    explicit NamePredicate(std::string pattern);

    bool Evaluate(Context& ctx, const OEChem::OEAtomBase& atom) const override;
    [[nodiscard]] std::string ToCanonical() const override;
    [[nodiscard]] PredicateType Type() const override { return PredicateType::Name; }

private:
    std::string pattern_;    ///< Name pattern (may contain wildcards)
    bool has_wildcard_;      ///< True if pattern contains * or ?
};

}  // namespace OESel

#endif  // OESELECT_PREDICATES_NAME_PREDICATE_H
