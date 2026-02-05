// include/oeselect/predicates/ExpansionPredicates.h
#ifndef OESELECT_PREDICATES_EXPANSION_PREDICATES_H
#define OESELECT_PREDICATES_EXPANSION_PREDICATES_H

#include "oeselect/Predicate.h"
#include <unordered_set>

namespace OESel {

/// Expands selection to complete residues
/// Selects all atoms in residues that have at least one atom matching the child selection
class ByResPredicate : public Predicate {
public:
    explicit ByResPredicate(Ptr child);

    bool Evaluate(Context& ctx, const OEChem::OEAtomBase& atom) const override;
    std::string ToCanonical() const override;
    PredicateType Type() const override { return PredicateType::ByRes; }
    std::vector<Ptr> Children() const override { return {child_}; }

private:
    Ptr child_;

    /// Get set of residue indices that match the child selection
    const std::unordered_set<unsigned int>& GetMatchingResidues(Context& ctx) const;
};

/// Expands selection to complete chains
/// Selects all atoms in chains that have at least one atom matching the child selection
class ByChainPredicate : public Predicate {
public:
    explicit ByChainPredicate(Ptr child);

    bool Evaluate(Context& ctx, const OEChem::OEAtomBase& atom) const override;
    std::string ToCanonical() const override;
    PredicateType Type() const override { return PredicateType::ByChain; }
    std::vector<Ptr> Children() const override { return {child_}; }

private:
    Ptr child_;

    /// Get set of atom indices that are in matching chains
    const std::unordered_set<unsigned int>& GetMatchingChainAtoms(Context& ctx) const;
};

}  // namespace OESel

#endif  // OESELECT_PREDICATES_EXPANSION_PREDICATES_H
