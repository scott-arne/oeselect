// include/oeselect/Predicate.h
#ifndef OESELECT_PREDICATE_H
#define OESELECT_PREDICATE_H

#include <memory>
#include <string>
#include <vector>

namespace OEChem {
class OEAtomBase;
}

namespace OESel {

class Context;

/// Predicate type enumeration for introspection
enum class PredicateType {
    // Logical
    And, Or, Not, XOr,
    // Atom properties
    Name, Resn, Resi, Chain, Elem, Index, SecondaryStructure,
    // Molecular components
    Protein, Ligand, Water, Solvent, Organic, Backbone, Metal,
    // Atom types
    Heavy, Hydrogen, PolarHydrogen, NonpolarHydrogen,
    // Expansion
    ByRes, ByChain,
    // Distance
    Around, XAround, Box, XBox, Beyond,
    // Constants
    True, False
};

/// Base class for all predicates
class Predicate {
public:
    using Ptr = std::shared_ptr<Predicate>;

    virtual ~Predicate() = default;

    /// Evaluate predicate for an atom in context
    virtual bool Evaluate(Context& ctx, const OEChem::OEAtomBase& atom) const = 0;

    /// Get canonical string representation
    virtual std::string ToCanonical() const = 0;

    /// Get predicate type for introspection
    virtual PredicateType Type() const = 0;

    /// Get child predicates (for composite predicates)
    virtual std::vector<Ptr> Children() const { return {}; }
};

}  // namespace OESel

#endif  // OESELECT_PREDICATE_H
