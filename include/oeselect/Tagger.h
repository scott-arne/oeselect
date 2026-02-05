/**
 * @file Tagger.h
 * @brief Molecular component classification and tagging.
 *
 * The Tagger class provides automatic classification of atoms into molecular
 * components (protein, ligand, water, etc.) based on residue names. Tags are
 * stored as atom data and cached on the molecule.
 */

#ifndef OESELECT_TAGGER_H
#define OESELECT_TAGGER_H

#include <cstdint>

namespace OEChem {
class OEMolBase;
class OEAtomBase;
}

namespace OESel {

/**
 * @brief Component flags for molecular classification.
 *
 * Each flag represents a molecular component type. Flags can be combined
 * using bitwise operators for atoms that belong to multiple categories.
 */
enum class ComponentFlag : uint32_t {
    None     = 0,       ///< No component assignment
    Protein  = 1 << 0,  ///< Standard amino acid residues
    Ligand   = 1 << 1,  ///< Small molecule ligands (default for unknowns)
    Solvent  = 1 << 2,  ///< Common solvents (DMSO, DMF, etc.)
    Cofactor = 1 << 3,  ///< Enzyme cofactors (NAD, FAD, etc.)
    Nucleic  = 1 << 4,  ///< Nucleic acid residues
    Water    = 1 << 5,  ///< Water molecules (HOH, WAT, etc.)
};

/**
 * @brief Bitwise OR operator for combining component flags.
 * @param a First flag.
 * @param b Second flag.
 * @return Combined flags.
 */
inline ComponentFlag operator|(ComponentFlag a, ComponentFlag b) {
    return static_cast<ComponentFlag>(
        static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

/**
 * @brief Bitwise AND operator for testing component flags.
 * @param a First flag.
 * @param b Second flag.
 * @return Intersection of flags.
 */
inline ComponentFlag operator&(ComponentFlag a, ComponentFlag b) {
    return static_cast<ComponentFlag>(
        static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

/**
 * @brief Utility for tagging molecules with component classifications.
 *
 * Tagger analyzes residue names to classify atoms into component types.
 * Classification is performed once per molecule and cached using OEMolBase
 * generic data. Subsequent calls to TagMolecule() are no-ops.
 *
 * @note Classification is based on residue names and may not be accurate
 *       for non-standard naming conventions.
 */
class Tagger {
public:
    /**
     * @brief Tag all atoms in a molecule with component flags.
     *
     * Classifies each atom based on its residue name and stores the
     * component flag as atom generic data. This operation is idempotent;
     * calling it multiple times has no additional effect.
     *
     * @param mol The molecule to tag.
     */
    static void TagMolecule(OEChem::OEMolBase& mol);

    /**
     * @brief Check if an atom has a specific component flag.
     *
     * @param atom The atom to check.
     * @param flag The component flag to test.
     * @return true if the atom has the specified flag.
     */
    static bool HasComponent(const OEChem::OEAtomBase& atom, ComponentFlag flag);

    /**
     * @brief Get the raw component flags for an atom.
     *
     * @param atom The atom to query.
     * @return Bitfield of component flags, or 0 if not tagged.
     */
    static uint32_t GetFlags(const OEChem::OEAtomBase& atom);

    /**
     * @brief Check if a molecule has been tagged.
     *
     * @param mol The molecule to check.
     * @return true if TagMolecule() has been called on this molecule.
     */
    static bool IsTagged(const OEChem::OEMolBase& mol);
};

}  // namespace OESel

#endif  // OESELECT_TAGGER_H
