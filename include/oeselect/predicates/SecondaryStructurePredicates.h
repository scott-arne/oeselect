/**
 * @file SecondaryStructurePredicates.h
 * @brief Secondary structure predicates (helix, sheet, turn, loop).
 *
 * These predicates select atoms based on the secondary structure
 * assignment of their residues. Secondary structure must be assigned
 * prior to selection (e.g., from PDB HELIX/SHEET records or DSSP).
 */

#ifndef OESELECT_PREDICATES_SECONDARY_STRUCTURE_PREDICATES_H
#define OESELECT_PREDICATES_SECONDARY_STRUCTURE_PREDICATES_H

#include "oeselect/Predicate.h"

namespace OESel {

/**
 * @brief Selects atoms in alpha helix secondary structure.
 *
 * Matches atoms in residues with OESecondaryStructure::Helix assignment.
 */
class HelixPredicate : public Predicate {
public:
    bool Evaluate(Context& ctx, const OEChem::OEAtomBase& atom) const override;
    std::string ToCanonical() const override { return "helix"; }
    PredicateType Type() const override { return PredicateType::Helix; }
};

/**
 * @brief Selects atoms in beta sheet secondary structure.
 *
 * Matches atoms in residues with OESecondaryStructure::Sheet assignment.
 */
class SheetPredicate : public Predicate {
public:
    bool Evaluate(Context& ctx, const OEChem::OEAtomBase& atom) const override;
    std::string ToCanonical() const override { return "sheet"; }
    PredicateType Type() const override { return PredicateType::Sheet; }
};

/**
 * @brief Selects atoms in turn secondary structure.
 *
 * Matches atoms in residues with OESecondaryStructure::Turn assignment.
 */
class TurnPredicate : public Predicate {
public:
    bool Evaluate(Context& ctx, const OEChem::OEAtomBase& atom) const override;
    std::string ToCanonical() const override { return "turn"; }
    PredicateType Type() const override { return PredicateType::Turn; }
};

/**
 * @brief Selects atoms in loop/coil secondary structure.
 *
 * Matches atoms in residues that are NOT assigned as helix, sheet, or turn.
 * This includes random coil and any unassigned residues.
 */
class LoopPredicate : public Predicate {
public:
    bool Evaluate(Context& ctx, const OEChem::OEAtomBase& atom) const override;
    std::string ToCanonical() const override { return "loop"; }
    PredicateType Type() const override { return PredicateType::Loop; }
};

}  // namespace OESel

#endif  // OESELECT_PREDICATES_SECONDARY_STRUCTURE_PREDICATES_H
