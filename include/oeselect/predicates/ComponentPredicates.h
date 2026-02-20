/**
 * @file ComponentPredicates.h
 * @brief Molecular component predicates (protein, ligand, water, etc.).
 *
 * These predicates classify atoms based on their molecular context,
 * using the Tagger system for residue-based classification.
 */

#ifndef OESELECT_PREDICATES_COMPONENT_PREDICATES_H
#define OESELECT_PREDICATES_COMPONENT_PREDICATES_H

#include "oeselect/Predicate.h"

namespace OESel {

/**
 * @brief Matches atoms in protein (amino acid) residues.
 *
 * Uses standard three-letter amino acid codes including common
 * protonation states (HID, HIE, HIP) and modifications.
 */
class ProteinPredicate : public Predicate {
public:
    bool Evaluate(Context& ctx, const OEChem::OEAtomBase& atom) const override;
    [[nodiscard]] std::string ToCanonical() const override { return "protein"; }
    [[nodiscard]] PredicateType Type() const override { return PredicateType::Protein; }
};

/**
 * @brief Matches atoms in small molecule ligands.
 *
 * A ligand is any residue that is not classified as protein, nucleic
 * acid, water, solvent, or cofactor. This is the default classification
 * for unknown residue names.
 */
class LigandPredicate : public Predicate {
public:
    bool Evaluate(Context& ctx, const OEChem::OEAtomBase& atom) const override;
    [[nodiscard]] std::string ToCanonical() const override { return "ligand"; }
    [[nodiscard]] PredicateType Type() const override { return PredicateType::Ligand; }
};

/**
 * @brief Matches atoms in water molecules.
 *
 * Recognizes common water residue names: HOH, WAT, H2O, DOD, TIP, TIP3, SPC.
 */
class WaterPredicate : public Predicate {
public:
    bool Evaluate(Context& ctx, const OEChem::OEAtomBase& atom) const override;
    [[nodiscard]] std::string ToCanonical() const override { return "water"; }
    [[nodiscard]] PredicateType Type() const override { return PredicateType::Water; }
};

/**
 * @brief Matches atoms in solvent molecules (water + common solvents).
 *
 * Includes water plus common organic solvents: DMSO, DMF, acetonitrile,
 * methanol, ethanol, isopropanol, glycerol, PEG, ethylene glycol.
 */
class SolventPredicate : public Predicate {
public:
    bool Evaluate(Context& ctx, const OEChem::OEAtomBase& atom) const override;
    [[nodiscard]] std::string ToCanonical() const override { return "solvent"; }
    [[nodiscard]] PredicateType Type() const override { return PredicateType::Solvent; }
};

/**
 * @brief Matches atoms in organic molecules.
 *
 * Organic atoms are carbon-containing atoms (or atoms bonded to carbon)
 * that are not part of protein or nucleic acid residues.
 */
class OrganicPredicate : public Predicate {
public:
    bool Evaluate(Context& ctx, const OEChem::OEAtomBase& atom) const override;
    [[nodiscard]] std::string ToCanonical() const override { return "organic"; }
    [[nodiscard]] PredicateType Type() const override { return PredicateType::Organic; }
};

/**
 * @brief Matches protein backbone atoms (N, CA, C, O).
 *
 * Only matches atoms in protein residues with backbone atom names.
 */
class BackbonePredicate : public Predicate {
public:
    bool Evaluate(Context& ctx, const OEChem::OEAtomBase& atom) const override;
    [[nodiscard]] std::string ToCanonical() const override { return "backbone"; }
    [[nodiscard]] PredicateType Type() const override { return PredicateType::Backbone; }
};

/**
 * @brief Matches protein sidechain atoms.
 *
 * Matches atoms in protein residues that are not backbone atoms
 * (N, CA, C, O) or terminal oxygen (OXT).
 */
class SidechainPredicate : public Predicate {
public:
    bool Evaluate(Context& ctx, const OEChem::OEAtomBase& atom) const override;
    [[nodiscard]] std::string ToCanonical() const override { return "sidechain"; }
    [[nodiscard]] PredicateType Type() const override { return PredicateType::Backbone; }  // Reuses type
};

/**
 * @brief Matches metal ions.
 *
 * Uses atomic number ranges to identify common biologically relevant
 * metals: alkali metals, alkaline earth metals, and transition metals.
 */
class MetalPredicate : public Predicate {
public:
    bool Evaluate(Context& ctx, const OEChem::OEAtomBase& atom) const override;
    [[nodiscard]] std::string ToCanonical() const override { return "metal"; }
    [[nodiscard]] PredicateType Type() const override { return PredicateType::Metal; }
};

}  // namespace OESel

#endif  // OESELECT_PREDICATES_COMPONENT_PREDICATES_H
