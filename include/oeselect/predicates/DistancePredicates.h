// include/oeselect/predicates/DistancePredicates.h
#ifndef OESELECT_PREDICATES_DISTANCE_PREDICATES_H
#define OESELECT_PREDICATES_DISTANCE_PREDICATES_H

#include "oeselect/Predicate.h"

namespace OESel {

/// Matches atoms within radius of any atom in reference selection
class AroundPredicate : public Predicate {
public:
    AroundPredicate(float radius, Ptr reference);

    bool Evaluate(Context& ctx, const OEChem::OEAtomBase& atom) const override;
    std::string ToCanonical() const override;
    PredicateType Type() const override { return PredicateType::Around; }
    std::vector<Ptr> Children() const override { return {reference_}; }

private:
    float radius_;
    Ptr reference_;

    /// Helper to compute and cache the around atom mask
    const std::vector<bool>& GetAroundMask(Context& ctx) const;
};

/// Matches atoms within radius of reference, excluding reference atoms themselves
class XAroundPredicate : public Predicate {
public:
    XAroundPredicate(float radius, Ptr reference);

    bool Evaluate(Context& ctx, const OEChem::OEAtomBase& atom) const override;
    std::string ToCanonical() const override;
    PredicateType Type() const override { return PredicateType::XAround; }
    std::vector<Ptr> Children() const override { return {reference_}; }

private:
    float radius_;
    Ptr reference_;

    /// Helper to compute and cache the around atom mask (same as AroundPredicate)
    const std::vector<bool>& GetAroundMask(Context& ctx) const;
};

/// Matches atoms outside radius of all atoms in reference selection
class BeyondPredicate : public Predicate {
public:
    BeyondPredicate(float radius, Ptr reference);

    bool Evaluate(Context& ctx, const OEChem::OEAtomBase& atom) const override;
    std::string ToCanonical() const override;
    PredicateType Type() const override { return PredicateType::Beyond; }
    std::vector<Ptr> Children() const override { return {reference_}; }

private:
    float radius_;
    Ptr reference_;

    /// Helper to compute and cache the around atom mask (inverse of around)
    const std::vector<bool>& GetAroundMask(Context& ctx) const;
};

}  // namespace OESel

#endif  // OESELECT_PREDICATES_DISTANCE_PREDICATES_H
