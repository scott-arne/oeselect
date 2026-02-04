// include/oeselect/Tagger.h
#ifndef OESELECT_TAGGER_H
#define OESELECT_TAGGER_H

#include <cstdint>

namespace OEChem {
class OEMolBase;
class OEAtomBase;
}

namespace OESel {

/// Component flags for molecular classification
enum class ComponentFlag : uint32_t {
    None     = 0,
    Protein  = 1 << 0,
    Ligand   = 1 << 1,
    Solvent  = 1 << 2,
    Cofactor = 1 << 3,
    Nucleic  = 1 << 4,
    Water    = 1 << 5,
};

/// Bitwise OR for ComponentFlag
inline ComponentFlag operator|(ComponentFlag a, ComponentFlag b) {
    return static_cast<ComponentFlag>(
        static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

/// Bitwise AND for ComponentFlag
inline ComponentFlag operator&(ComponentFlag a, ComponentFlag b) {
    return static_cast<ComponentFlag>(
        static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

/// Tag molecules with component classifications
class Tagger {
public:
    /// Tag all atoms in molecule (idempotent)
    static void TagMolecule(OEChem::OEMolBase& mol);

    /// Check if atom has component flag
    static bool HasComponent(const OEChem::OEAtomBase& atom, ComponentFlag flag);

    /// Get raw component flags for atom
    static uint32_t GetFlags(const OEChem::OEAtomBase& atom);

    /// Check if molecule has been tagged
    static bool IsTagged(const OEChem::OEMolBase& mol);
};

}  // namespace OESel

#endif  // OESELECT_TAGGER_H
