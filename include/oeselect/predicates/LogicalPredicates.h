/**
 * @file LogicalPredicates.h
 * @brief Logical combination predicates (AND, OR, NOT, XOR).
 *
 * These predicates combine child predicates using boolean logic to create
 * complex selection expressions.
 */

#ifndef OESELECT_PREDICATES_LOGICAL_PREDICATES_H
#define OESELECT_PREDICATES_LOGICAL_PREDICATES_H

#include "oeselect/Predicate.h"
#include <vector>

namespace OESel {

/**
 * @brief Logical AND predicate - all children must match.
 *
 * Evaluates children in order and short-circuits on first false result.
 * An empty AND predicate matches all atoms.
 */
class AndPredicate : public Predicate {
public:
    /**
     * @brief Construct AND predicate from children.
     * @param children Child predicates to combine.
     */
    explicit AndPredicate(std::vector<Ptr> children);

    bool Evaluate(Context& ctx, const OEChem::OEAtomBase& atom) const override;
    [[nodiscard]] std::string ToCanonical() const override;
    [[nodiscard]] PredicateType Type() const override { return PredicateType::And; }
    [[nodiscard]] std::vector<Ptr> Children() const override { return children_; }

private:
    std::vector<Ptr> children_;
};

/**
 * @brief Logical OR predicate - any child must match.
 *
 * Evaluates children in order and short-circuits on first true result.
 * An empty OR predicate matches no atoms.
 */
class OrPredicate : public Predicate {
public:
    /**
     * @brief Construct OR predicate from children.
     * @param children Child predicates to combine.
     */
    explicit OrPredicate(std::vector<Ptr> children);

    bool Evaluate(Context& ctx, const OEChem::OEAtomBase& atom) const override;
    [[nodiscard]] std::string ToCanonical() const override;
    [[nodiscard]] PredicateType Type() const override { return PredicateType::Or; }
    [[nodiscard]] std::vector<Ptr> Children() const override { return children_; }

private:
    std::vector<Ptr> children_;
};

/**
 * @brief Logical NOT predicate - inverts child result.
 *
 * Returns true if and only if the child predicate returns false.
 */
class NotPredicate : public Predicate {
public:
    /**
     * @brief Construct NOT predicate.
     * @param child The predicate to negate.
     */
    explicit NotPredicate(Ptr child);

    bool Evaluate(Context& ctx, const OEChem::OEAtomBase& atom) const override;
    [[nodiscard]] std::string ToCanonical() const override;
    [[nodiscard]] PredicateType Type() const override { return PredicateType::Not; }
    [[nodiscard]] std::vector<Ptr> Children() const override { return {child_}; }

private:
    Ptr child_;
};

/**
 * @brief Logical XOR predicate - exactly one child must match.
 *
 * Returns true if exactly one of the child predicates matches.
 * Evaluates all children (no short-circuit optimization).
 */
class XOrPredicate : public Predicate {
public:
    /**
     * @brief Construct XOR predicate from children.
     * @param children Child predicates to combine.
     */
    explicit XOrPredicate(std::vector<Ptr> children);

    bool Evaluate(Context& ctx, const OEChem::OEAtomBase& atom) const override;
    [[nodiscard]] std::string ToCanonical() const override;
    [[nodiscard]] PredicateType Type() const override { return PredicateType::XOr; }
    [[nodiscard]] std::vector<Ptr> Children() const override { return children_; }

private:
    std::vector<Ptr> children_;
};

}  // namespace OESel

#endif  // OESELECT_PREDICATES_LOGICAL_PREDICATES_H
