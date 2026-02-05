// include/oeselect/predicates/SecondaryStructurePredicates.h
#ifndef OESELECT_PREDICATES_SECONDARY_STRUCTURE_PREDICATES_H
#define OESELECT_PREDICATES_SECONDARY_STRUCTURE_PREDICATES_H

#include "oeselect/Predicate.h"

namespace OESel {

/// Selects atoms in alpha helix secondary structure
class HelixPredicate : public Predicate {
public:
    bool Evaluate(Context& ctx, const OEChem::OEAtomBase& atom) const override;
    std::string ToCanonical() const override { return "helix"; }
    PredicateType Type() const override { return PredicateType::Helix; }
};

/// Selects atoms in beta sheet secondary structure
class SheetPredicate : public Predicate {
public:
    bool Evaluate(Context& ctx, const OEChem::OEAtomBase& atom) const override;
    std::string ToCanonical() const override { return "sheet"; }
    PredicateType Type() const override { return PredicateType::Sheet; }
};

/// Selects atoms in turn secondary structure
class TurnPredicate : public Predicate {
public:
    bool Evaluate(Context& ctx, const OEChem::OEAtomBase& atom) const override;
    std::string ToCanonical() const override { return "turn"; }
    PredicateType Type() const override { return PredicateType::Turn; }
};

/// Selects atoms in loop/coil secondary structure (not helix, sheet, or turn)
class LoopPredicate : public Predicate {
public:
    bool Evaluate(Context& ctx, const OEChem::OEAtomBase& atom) const override;
    std::string ToCanonical() const override { return "loop"; }
    PredicateType Type() const override { return PredicateType::Loop; }
};

}  // namespace OESel

#endif  // OESELECT_PREDICATES_SECONDARY_STRUCTURE_PREDICATES_H
