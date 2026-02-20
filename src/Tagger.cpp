/**
 * @file Tagger.cpp
 * @brief Molecular component tagging implementation.
 *
 * Classifies atoms into molecular components based on residue names.
 * Uses OEChem generic data tags for efficient storage and retrieval.
 */

#include "oeselect/Tagger.h"

#include <oechem.h>

#include <set>
#include <string>

namespace OESel {

namespace {
// Lazy-initialized global tags for atom and molecule data
unsigned int g_component_tag = 0;
unsigned int g_tagged_tag = 0;

unsigned int GetComponentTag() {
    if (g_component_tag == 0) {
        g_component_tag = OESystem::OEGetTag("OESel_Component");
    }
    return g_component_tag;
}

unsigned int GetTaggedTag() {
    if (g_tagged_tag == 0) {
        g_tagged_tag = OESystem::OEGetTag("OESel_Tagged");
    }
    return g_tagged_tag;
}

// Standard residue name sets for classification
const std::set<std::string> WATER_RESNAMES = {
    "HOH", "WAT", "H2O", "DOD", "TIP", "TIP3", "SPC"
};

const std::set<std::string> AMINO_ACIDS = {
    "ALA", "ARG", "ASN", "ASP", "CYS", "GLN", "GLU", "GLY", "HIS", "ILE",
    "LEU", "LYS", "MET", "PHE", "PRO", "SER", "THR", "TRP", "TYR", "VAL",
    // Common protonation states and modifications
    "HID", "HIE", "HIP", "CYX", "ASH", "GLH",
    // Terminal capping groups
    "ACE", "NME"
};

const std::set<std::string> NUCLEOTIDES = {
    "A", "G", "C", "U", "T",
    "DA", "DG", "DC", "DT", "DU",
    "ADE", "GUA", "CYT", "URA", "THY",
    "RA", "RG", "RC", "RU"
};

const std::set<std::string> COFACTORS = {
    "NAD", "NAP", "NAI", "NDP",   // NAD variants
    "FAD", "FMN", "FNR",          // Flavin cofactors
    "HEM", "HEC", "HEA",          // Heme variants
    "ATP", "ADP", "AMP",          // Adenine nucleotides
    "GTP", "GDP", "GMP",          // Guanine nucleotides
    "COA", "ACO",                 // Coenzyme A
    "PLP",                        // Pyridoxal phosphate
    "BTN",                        // Biotin
    "B12", "CBY",                 // Vitamin B12
    "SF4", "FES", "F3S",          // Iron-sulfur clusters
    "MG", "CA", "ZN", "FE", "MN", "CU"  // Common metal cofactors
};

const std::set<std::string> SOLVENTS = {
    "DMS", "DMF", "ACN", "MET", "EOH", "IPA", "GOL", "PEG", "EDO"
};

/**
 * @brief Remove leading and trailing whitespace from a string.
 */
std::string TrimWhitespace(const std::string& str) {
    std::string result = str;
    while (!result.empty() && std::isspace(static_cast<unsigned char>(result.back()))) {
        result.pop_back();
    }
    size_t start = 0;
    while (start < result.size() && std::isspace(static_cast<unsigned char>(result[start]))) {
        ++start;
    }
    return result.substr(start);
}

/**
 * @brief Classify a residue by its name.
 *
 * @param resname The residue name to classify.
 * @return Component flag indicating the residue type.
 */
ComponentFlag ClassifyResidue(const std::string& resname) {
    const std::string name = TrimWhitespace(resname);

    if (WATER_RESNAMES.count(name)) return ComponentFlag::Water;
    if (AMINO_ACIDS.count(name)) return ComponentFlag::Protein;
    if (NUCLEOTIDES.count(name)) return ComponentFlag::Nucleic;
    if (COFACTORS.count(name)) return ComponentFlag::Cofactor;
    if (SOLVENTS.count(name)) return ComponentFlag::Solvent;

    // Unknown residues default to ligand
    return ComponentFlag::Ligand;
}
}  // namespace

void Tagger::TagMolecule(OEChem::OEMolBase& mol) {
    if (IsTagged(mol)) {
        return;  // Idempotent - skip if already tagged
    }

    // Iterate all atoms and assign component flags based on residue name
    for (OESystem::OEIter atom = mol.GetAtoms(); atom; ++atom) {
        const OEChem::OEResidue& res = OEChem::OEAtomGetResidue(&*atom);
        const std::string resname = res.GetName();

        ComponentFlag flag = ClassifyResidue(resname);
        atom->SetData<unsigned int>(GetComponentTag(), static_cast<uint32_t>(flag));
    }

    // Mark molecule as tagged to prevent redundant processing
    mol.SetData<unsigned int>(GetTaggedTag(), 1);
}

bool Tagger::HasComponent(const OEChem::OEAtomBase& atom, ComponentFlag flag) {
    const uint32_t flags = GetFlags(atom);
    return (flags & static_cast<uint32_t>(flag)) != 0;
}

uint32_t Tagger::GetFlags(const OEChem::OEAtomBase& atom) {
    if (!atom.HasData(GetComponentTag())) {
        return 0;
    }
    return atom.GetData<unsigned int>(GetComponentTag());
}

bool Tagger::IsTagged(const OEChem::OEMolBase& mol) {
    return mol.HasData(GetTaggedTag());
}

}  // namespace OESel
