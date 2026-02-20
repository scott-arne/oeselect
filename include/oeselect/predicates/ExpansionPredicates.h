/**
 * @file ExpansionPredicates.h
 * @brief Selection expansion predicates (byres, bychain).
 *
 * These predicates expand a selection to include complete structural
 * units (residues or chains) containing any matching atoms.
 */

#ifndef OESELECT_PREDICATES_EXPANSION_PREDICATES_H
#define OESELECT_PREDICATES_EXPANSION_PREDICATES_H

#include "oeselect/Predicate.h"
#include <unordered_set>

namespace OESel {

/**
 * @brief Expands selection to complete residues.
 *
 * Selects all atoms in any residue that contains at least one atom
 * matching the child selection. Residue identity is determined by
 * chain ID, residue number, and insertion code.
 *
 * @code
 * // Selection: byres name CA
 * // Selects all atoms in residues that have a CA atom
 * @endcode
 */
class ByResPredicate : public Predicate {
public:
    /**
     * @brief Construct residue expansion predicate.
     * @param child Selection to expand.
     */
    explicit ByResPredicate(Ptr child);

    bool Evaluate(Context& ctx, const OEChem::OEAtomBase& atom) const override;
    [[nodiscard]] std::string ToCanonical() const override;
    [[nodiscard]] PredicateType Type() const override { return PredicateType::ByRes; }
    [[nodiscard]] std::vector<Ptr> Children() const override { return {child_}; }

private:
    Ptr child_;

    /// Get cached set of atom indices in matching residues
    const std::unordered_set<unsigned int>& GetMatchingResidues(Context& ctx) const;
};

/**
 * @brief Expands selection to complete chains.
 *
 * Selects all atoms in any chain that contains at least one atom
 * matching the child selection. Chain identity is determined by
 * chain ID only.
 *
 * @code
 * // Selection: bychain ligand
 * // Selects all atoms in chains that contain a ligand
 * @endcode
 */
class ByChainPredicate : public Predicate {
public:
    /**
     * @brief Construct chain expansion predicate.
     * @param child Selection to expand.
     */
    explicit ByChainPredicate(Ptr child);

    bool Evaluate(Context& ctx, const OEChem::OEAtomBase& atom) const override;
    [[nodiscard]] std::string ToCanonical() const override;
    [[nodiscard]] PredicateType Type() const override { return PredicateType::ByChain; }
    [[nodiscard]] std::vector<Ptr> Children() const override { return {child_}; }

private:
    Ptr child_;

    /// Get cached set of atom indices in matching chains
    const std::unordered_set<unsigned int>& GetMatchingChainAtoms(Context& ctx) const;
};

}  // namespace OESel

#endif  // OESELECT_PREDICATES_EXPANSION_PREDICATES_H
