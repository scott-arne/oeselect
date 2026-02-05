/**
 * @file DistancePredicates.h
 * @brief Distance-based selection predicates.
 *
 * These predicates select atoms based on spatial proximity to a reference
 * selection. They use a k-d tree spatial index for efficient queries.
 */

#ifndef OESELECT_PREDICATES_DISTANCE_PREDICATES_H
#define OESELECT_PREDICATES_DISTANCE_PREDICATES_H

#include "oeselect/Predicate.h"

namespace OESel {

/**
 * @brief Selects atoms within a distance of a reference selection.
 *
 * Matches any atom that is within the specified radius of at least one
 * atom in the reference selection. The reference atoms themselves are
 * included in the result.
 *
 * @code
 * // Selection: around 5.0 ligand
 * // Matches all atoms within 5 Angstroms of any ligand atom
 * @endcode
 */
class AroundPredicate : public Predicate {
public:
    /**
     * @brief Construct around predicate.
     * @param radius Distance threshold in Angstroms.
     * @param reference Reference selection for distance calculation.
     */
    AroundPredicate(float radius, Ptr reference);

    bool Evaluate(Context& ctx, const OEChem::OEAtomBase& atom) const override;
    std::string ToCanonical() const override;
    PredicateType Type() const override { return PredicateType::Around; }
    std::vector<Ptr> Children() const override { return {reference_}; }

private:
    float radius_;
    Ptr reference_;

    /// Compute and cache the boolean mask of nearby atoms
    const std::vector<bool>& GetAroundMask(Context& ctx) const;
};

/**
 * @brief Selects atoms within distance, excluding reference atoms.
 *
 * Similar to AroundPredicate, but excludes atoms that are part of the
 * reference selection. Useful for finding the environment around a
 * selection without including the selection itself.
 *
 * @code
 * // Selection: xaround 5.0 ligand
 * // Matches atoms within 5A of ligand, but not ligand atoms themselves
 * @endcode
 */
class XAroundPredicate : public Predicate {
public:
    /**
     * @brief Construct exclusive around predicate.
     * @param radius Distance threshold in Angstroms.
     * @param reference Reference selection for distance calculation.
     */
    XAroundPredicate(float radius, Ptr reference);

    bool Evaluate(Context& ctx, const OEChem::OEAtomBase& atom) const override;
    std::string ToCanonical() const override;
    PredicateType Type() const override { return PredicateType::XAround; }
    std::vector<Ptr> Children() const override { return {reference_}; }

private:
    float radius_;
    Ptr reference_;

    /// Compute and cache the boolean mask (shared with AroundPredicate)
    const std::vector<bool>& GetAroundMask(Context& ctx) const;
};

/**
 * @brief Selects atoms beyond a distance from a reference selection.
 *
 * Matches atoms that are farther than the specified radius from ALL
 * atoms in the reference selection. This is the logical inverse of
 * AroundPredicate.
 *
 * @code
 * // Selection: beyond 10.0 protein
 * // Matches atoms more than 10A away from any protein atom
 * @endcode
 */
class BeyondPredicate : public Predicate {
public:
    /**
     * @brief Construct beyond predicate.
     * @param radius Distance threshold in Angstroms.
     * @param reference Reference selection for distance calculation.
     */
    BeyondPredicate(float radius, Ptr reference);

    bool Evaluate(Context& ctx, const OEChem::OEAtomBase& atom) const override;
    std::string ToCanonical() const override;
    PredicateType Type() const override { return PredicateType::Beyond; }
    std::vector<Ptr> Children() const override { return {reference_}; }

private:
    float radius_;
    Ptr reference_;

    /// Compute and cache the around mask (inverted for beyond logic)
    const std::vector<bool>& GetAroundMask(Context& ctx) const;
};

}  // namespace OESel

#endif  // OESELECT_PREDICATES_DISTANCE_PREDICATES_H
