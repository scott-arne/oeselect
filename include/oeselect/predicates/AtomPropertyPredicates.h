/**
 * @file AtomPropertyPredicates.h
 * @brief Atom property predicates (residue name, number, chain, element, index).
 *
 * These predicates match atoms based on their structural properties
 * from the PDB hierarchy (residue, chain) or intrinsic properties (element).
 */

#ifndef OESELECT_PREDICATES_ATOM_PROPERTY_PREDICATES_H
#define OESELECT_PREDICATES_ATOM_PROPERTY_PREDICATES_H

#include "oeselect/Predicate.h"
#include <string>

namespace OESel {

/**
 * @brief Matches atoms by residue name.
 *
 * Supports exact matching or glob-style wildcards (* and ?).
 *
 * @code
 * // Selection: resn ALA
 * // Selection: resn AL*  (matches ALA, ALX, etc.)
 * @endcode
 */
class ResnPredicate : public Predicate {
public:
    /**
     * @brief Construct residue name predicate.
     * @param pattern Residue name or glob pattern.
     */
    explicit ResnPredicate(std::string pattern);

    bool Evaluate(Context& ctx, const OEChem::OEAtomBase& atom) const override;
    std::string ToCanonical() const override;
    PredicateType Type() const override { return PredicateType::Resn; }

private:
    std::string pattern_;
    bool has_wildcard_;
};

/**
 * @brief Matches atoms by residue number.
 *
 * Supports exact match, comparisons (<, <=, >, >=), and ranges.
 *
 * @code
 * // Selection: resi 42      (exact match)
 * // Selection: resi 1-100   (range)
 * // Selection: resi > 50    (comparison)
 * @endcode
 */
class ResiPredicate : public Predicate {
public:
    /// Comparison operators for residue number matching
    enum class Op { Eq, Lt, Le, Gt, Ge, Range };

    /**
     * @brief Construct with single value and operator.
     * @param value Residue number to compare against.
     * @param op Comparison operator (default: exact match).
     */
    ResiPredicate(int value, Op op = Op::Eq);

    /**
     * @brief Construct with range.
     * @param start Start of range (inclusive).
     * @param end End of range (inclusive).
     */
    ResiPredicate(int start, int end);

    bool Evaluate(Context& ctx, const OEChem::OEAtomBase& atom) const override;
    std::string ToCanonical() const override;
    PredicateType Type() const override { return PredicateType::Resi; }

private:
    int value_;
    int end_value_;
    Op op_;
};

/**
 * @brief Matches atoms by chain identifier.
 *
 * Chain ID is a single character from the PDB hierarchy.
 *
 * @code
 * // Selection: chain A
 * @endcode
 */
class ChainPredicate : public Predicate {
public:
    /**
     * @brief Construct chain predicate.
     * @param chain_id Single-character chain identifier.
     */
    explicit ChainPredicate(std::string chain_id);

    bool Evaluate(Context& ctx, const OEChem::OEAtomBase& atom) const override;
    std::string ToCanonical() const override;
    PredicateType Type() const override { return PredicateType::Chain; }

private:
    std::string chain_id_;
};

/**
 * @brief Matches atoms by element symbol.
 *
 * Element symbols are case-insensitive and converted to atomic numbers
 * internally for efficient comparison.
 *
 * @code
 * // Selection: elem C   (carbon)
 * // Selection: elem Fe  (iron)
 * @endcode
 */
class ElemPredicate : public Predicate {
public:
    /**
     * @brief Construct element predicate.
     * @param element Element symbol (e.g., "C", "Fe").
     */
    explicit ElemPredicate(std::string element);

    bool Evaluate(Context& ctx, const OEChem::OEAtomBase& atom) const override;
    std::string ToCanonical() const override;
    PredicateType Type() const override { return PredicateType::Elem; }

private:
    unsigned int atomic_num_;  ///< Precomputed atomic number
    std::string element_;      ///< Normalized element symbol
};

/**
 * @brief Matches atoms by atom index.
 *
 * Supports exact match, comparisons, and ranges. Atom indices are
 * zero-based and molecule-specific.
 *
 * @code
 * // Selection: index 0       (first atom)
 * // Selection: index 0-99    (first 100 atoms)
 * // Selection: index >= 100  (atoms after first 100)
 * @endcode
 */
class IndexPredicate : public Predicate {
public:
    /// Comparison operators for index matching
    enum class Op { Eq, Lt, Le, Gt, Ge, Range };

    /**
     * @brief Construct with single value and operator.
     * @param value Atom index to compare against.
     * @param op Comparison operator (default: exact match).
     */
    IndexPredicate(unsigned int value, Op op = Op::Eq);

    /**
     * @brief Construct with range.
     * @param start Start of range (inclusive).
     * @param end End of range (inclusive).
     */
    IndexPredicate(unsigned int start, unsigned int end);

    bool Evaluate(Context& ctx, const OEChem::OEAtomBase& atom) const override;
    std::string ToCanonical() const override;
    PredicateType Type() const override { return PredicateType::Index; }

private:
    unsigned int value_;
    unsigned int end_value_;
    Op op_;
};

}  // namespace OESel

#endif  // OESELECT_PREDICATES_ATOM_PROPERTY_PREDICATES_H
