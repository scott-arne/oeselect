// include/oeselect/predicates/LogicalPredicates.h
#ifndef OESELECT_PREDICATES_LOGICAL_PREDICATES_H
#define OESELECT_PREDICATES_LOGICAL_PREDICATES_H

#include "oeselect/Predicate.h"
#include <vector>

namespace OESel {

/// AND predicate - all children must match
class AndPredicate : public Predicate {
public:
    explicit AndPredicate(std::vector<Ptr> children);

    bool Evaluate(Context& ctx, const OEChem::OEAtomBase& atom) const override;
    std::string ToCanonical() const override;
    PredicateType Type() const override { return PredicateType::And; }
    std::vector<Ptr> Children() const override { return children_; }

private:
    std::vector<Ptr> children_;
};

/// OR predicate - any child must match
class OrPredicate : public Predicate {
public:
    explicit OrPredicate(std::vector<Ptr> children);

    bool Evaluate(Context& ctx, const OEChem::OEAtomBase& atom) const override;
    std::string ToCanonical() const override;
    PredicateType Type() const override { return PredicateType::Or; }
    std::vector<Ptr> Children() const override { return children_; }

private:
    std::vector<Ptr> children_;
};

/// NOT predicate - inverts child
class NotPredicate : public Predicate {
public:
    explicit NotPredicate(Ptr child);

    bool Evaluate(Context& ctx, const OEChem::OEAtomBase& atom) const override;
    std::string ToCanonical() const override;
    PredicateType Type() const override { return PredicateType::Not; }
    std::vector<Ptr> Children() const override { return {child_}; }

private:
    Ptr child_;
};

/// XOR predicate - exactly one child must match
class XOrPredicate : public Predicate {
public:
    explicit XOrPredicate(std::vector<Ptr> children);

    bool Evaluate(Context& ctx, const OEChem::OEAtomBase& atom) const override;
    std::string ToCanonical() const override;
    PredicateType Type() const override { return PredicateType::XOr; }
    std::vector<Ptr> Children() const override { return children_; }

private:
    std::vector<Ptr> children_;
};

}  // namespace OESel

#endif  // OESELECT_PREDICATES_LOGICAL_PREDICATES_H
