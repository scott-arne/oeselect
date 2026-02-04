// include/oeselect/predicates/AtomPropertyPredicates.h
#ifndef OESELECT_PREDICATES_ATOM_PROPERTY_PREDICATES_H
#define OESELECT_PREDICATES_ATOM_PROPERTY_PREDICATES_H

#include "oeselect/Predicate.h"
#include <string>

namespace OESel {

/// Matches atoms by residue name with optional glob patterns
class ResnPredicate : public Predicate {
public:
    explicit ResnPredicate(std::string pattern);

    bool Evaluate(Context& ctx, const OEChem::OEAtomBase& atom) const override;
    std::string ToCanonical() const override;
    PredicateType Type() const override { return PredicateType::Resn; }

private:
    std::string pattern_;
    bool has_wildcard_;
};

/// Matches atoms by residue index with range/comparison support
class ResiPredicate : public Predicate {
public:
    enum class Op { Eq, Lt, Le, Gt, Ge, Range };

    ResiPredicate(int value, Op op = Op::Eq);
    ResiPredicate(int start, int end);  // Range constructor

    bool Evaluate(Context& ctx, const OEChem::OEAtomBase& atom) const override;
    std::string ToCanonical() const override;
    PredicateType Type() const override { return PredicateType::Resi; }

private:
    int value_;
    int end_value_;
    Op op_;
};

/// Matches atoms by chain ID
class ChainPredicate : public Predicate {
public:
    explicit ChainPredicate(std::string chain_id);

    bool Evaluate(Context& ctx, const OEChem::OEAtomBase& atom) const override;
    std::string ToCanonical() const override;
    PredicateType Type() const override { return PredicateType::Chain; }

private:
    std::string chain_id_;
};

/// Matches atoms by element symbol
class ElemPredicate : public Predicate {
public:
    explicit ElemPredicate(std::string element);

    bool Evaluate(Context& ctx, const OEChem::OEAtomBase& atom) const override;
    std::string ToCanonical() const override;
    PredicateType Type() const override { return PredicateType::Elem; }

private:
    unsigned int atomic_num_;
    std::string element_;
};

/// Matches atoms by atom index with range support
class IndexPredicate : public Predicate {
public:
    explicit IndexPredicate(unsigned int value);
    IndexPredicate(unsigned int start, unsigned int end);  // Range constructor

    bool Evaluate(Context& ctx, const OEChem::OEAtomBase& atom) const override;
    std::string ToCanonical() const override;
    PredicateType Type() const override { return PredicateType::Index; }

private:
    unsigned int value_;
    unsigned int end_value_;
    bool is_range_;
};

}  // namespace OESel

#endif  // OESELECT_PREDICATES_ATOM_PROPERTY_PREDICATES_H
