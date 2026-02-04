// include/oeselect/predicates/NamePredicate.h
#ifndef OESELECT_PREDICATES_NAME_PREDICATE_H
#define OESELECT_PREDICATES_NAME_PREDICATE_H

#include "oeselect/Predicate.h"
#include <string>

namespace OESel {

/// Matches atoms by name with optional glob patterns
class NamePredicate : public Predicate {
public:
    explicit NamePredicate(std::string pattern);

    bool Evaluate(Context& ctx, const OEChem::OEAtomBase& atom) const override;
    std::string ToCanonical() const override;
    PredicateType Type() const override { return PredicateType::Name; }

private:
    std::string pattern_;
    bool has_wildcard_;
};

}  // namespace OESel

#endif  // OESELECT_PREDICATES_NAME_PREDICATE_H
