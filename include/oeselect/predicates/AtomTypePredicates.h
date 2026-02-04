// include/oeselect/predicates/AtomTypePredicates.h
#ifndef OESELECT_PREDICATES_ATOM_TYPE_PREDICATES_H
#define OESELECT_PREDICATES_ATOM_TYPE_PREDICATES_H

#include "oeselect/Predicate.h"

namespace OESel {

/// Matches non-hydrogen atoms (atomic number > 1)
class HeavyPredicate : public Predicate {
public:
    bool Evaluate(Context& ctx, const OEChem::OEAtomBase& atom) const override;
    std::string ToCanonical() const override { return "heavy"; }
    PredicateType Type() const override { return PredicateType::Heavy; }
};

/// Matches hydrogen atoms (atomic number == 1)
class HydrogenPredicate : public Predicate {
public:
    bool Evaluate(Context& ctx, const OEChem::OEAtomBase& atom) const override;
    std::string ToCanonical() const override { return "hydrogen"; }
    PredicateType Type() const override { return PredicateType::Hydrogen; }
};

/// Matches polar hydrogens (H bonded to N, O, or S)
class PolarHydrogenPredicate : public Predicate {
public:
    bool Evaluate(Context& ctx, const OEChem::OEAtomBase& atom) const override;
    std::string ToCanonical() const override { return "polar_hydrogen"; }
    PredicateType Type() const override { return PredicateType::PolarHydrogen; }
};

/// Matches non-polar hydrogens (H bonded to C, or not bonded to N/O/S)
class NonpolarHydrogenPredicate : public Predicate {
public:
    bool Evaluate(Context& ctx, const OEChem::OEAtomBase& atom) const override;
    std::string ToCanonical() const override { return "nonpolar_hydrogen"; }
    PredicateType Type() const override { return PredicateType::NonpolarHydrogen; }
};

}  // namespace OESel

#endif  // OESELECT_PREDICATES_ATOM_TYPE_PREDICATES_H
