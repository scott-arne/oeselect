/**
 * @file AtomTypePredicates.h
 * @brief Atom type predicates based on element and bonding.
 *
 * These predicates classify atoms by their element type and bonding
 * environment (heavy atoms, hydrogens, polar/nonpolar).
 */

#ifndef OESELECT_PREDICATES_ATOM_TYPE_PREDICATES_H
#define OESELECT_PREDICATES_ATOM_TYPE_PREDICATES_H

#include "oeselect/Predicate.h"

namespace OESel {

/**
 * @brief Matches non-hydrogen atoms (heavy atoms).
 *
 * Returns true for any atom with atomic number > 1.
 */
class HeavyPredicate : public Predicate {
public:
    bool Evaluate(Context& ctx, const OEChem::OEAtomBase& atom) const override;
    std::string ToCanonical() const override { return "heavy"; }
    PredicateType Type() const override { return PredicateType::Heavy; }
};

/**
 * @brief Matches hydrogen atoms.
 *
 * Returns true for atoms with atomic number == 1.
 */
class HydrogenPredicate : public Predicate {
public:
    bool Evaluate(Context& ctx, const OEChem::OEAtomBase& atom) const override;
    std::string ToCanonical() const override { return "hydrogen"; }
    PredicateType Type() const override { return PredicateType::Hydrogen; }
};

/**
 * @brief Matches polar hydrogens (bonded to N, O, or S).
 *
 * Polar hydrogens participate in hydrogen bonding. This predicate
 * checks if the hydrogen is directly bonded to nitrogen (7),
 * oxygen (8), or sulfur (16).
 */
class PolarHydrogenPredicate : public Predicate {
public:
    bool Evaluate(Context& ctx, const OEChem::OEAtomBase& atom) const override;
    std::string ToCanonical() const override { return "polar_hydrogen"; }
    PredicateType Type() const override { return PredicateType::PolarHydrogen; }
};

/**
 * @brief Matches nonpolar hydrogens (not bonded to N, O, or S).
 *
 * Typically hydrogens bonded to carbon. More precisely, any hydrogen
 * that is not bonded to N, O, or S is considered nonpolar.
 */
class NonpolarHydrogenPredicate : public Predicate {
public:
    bool Evaluate(Context& ctx, const OEChem::OEAtomBase& atom) const override;
    std::string ToCanonical() const override { return "nonpolar_hydrogen"; }
    PredicateType Type() const override { return PredicateType::NonpolarHydrogen; }
};

}  // namespace OESel

#endif  // OESELECT_PREDICATES_ATOM_TYPE_PREDICATES_H
