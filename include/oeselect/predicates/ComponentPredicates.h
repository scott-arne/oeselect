// include/oeselect/predicates/ComponentPredicates.h
#ifndef OESELECT_PREDICATES_COMPONENT_PREDICATES_H
#define OESELECT_PREDICATES_COMPONENT_PREDICATES_H

#include "oeselect/Predicate.h"

namespace OESel {

/// Matches atoms in protein residues (amino acids)
class ProteinPredicate : public Predicate {
public:
    bool Evaluate(Context& ctx, const OEChem::OEAtomBase& atom) const override;
    std::string ToCanonical() const override { return "protein"; }
    PredicateType Type() const override { return PredicateType::Protein; }
};

/// Matches atoms in ligand molecules
class LigandPredicate : public Predicate {
public:
    bool Evaluate(Context& ctx, const OEChem::OEAtomBase& atom) const override;
    std::string ToCanonical() const override { return "ligand"; }
    PredicateType Type() const override { return PredicateType::Ligand; }
};

/// Matches atoms in water molecules
class WaterPredicate : public Predicate {
public:
    bool Evaluate(Context& ctx, const OEChem::OEAtomBase& atom) const override;
    std::string ToCanonical() const override { return "water"; }
    PredicateType Type() const override { return PredicateType::Water; }
};

/// Matches atoms in solvent molecules (including water)
class SolventPredicate : public Predicate {
public:
    bool Evaluate(Context& ctx, const OEChem::OEAtomBase& atom) const override;
    std::string ToCanonical() const override { return "solvent"; }
    PredicateType Type() const override { return PredicateType::Solvent; }
};

/// Matches atoms in organic molecules (C-containing, not protein/nucleic)
class OrganicPredicate : public Predicate {
public:
    bool Evaluate(Context& ctx, const OEChem::OEAtomBase& atom) const override;
    std::string ToCanonical() const override { return "organic"; }
    PredicateType Type() const override { return PredicateType::Organic; }
};

/// Matches protein backbone atoms (N, CA, C, O)
class BackbonePredicate : public Predicate {
public:
    bool Evaluate(Context& ctx, const OEChem::OEAtomBase& atom) const override;
    std::string ToCanonical() const override { return "backbone"; }
    PredicateType Type() const override { return PredicateType::Backbone; }
};

/// Matches protein sidechain atoms (non-backbone atoms in protein)
class SidechainPredicate : public Predicate {
public:
    bool Evaluate(Context& ctx, const OEChem::OEAtomBase& atom) const override;
    std::string ToCanonical() const override { return "sidechain"; }
    PredicateType Type() const override { return PredicateType::Backbone; }  // Reuse type
};

/// Matches metal ions
class MetalPredicate : public Predicate {
public:
    bool Evaluate(Context& ctx, const OEChem::OEAtomBase& atom) const override;
    std::string ToCanonical() const override { return "metal"; }
    PredicateType Type() const override { return PredicateType::Metal; }
};

}  // namespace OESel

#endif  // OESELECT_PREDICATES_COMPONENT_PREDICATES_H
