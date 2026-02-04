// src/Tagger.cpp
#include "oeselect/Tagger.h"

#include <oechem.h>

namespace OESel {

namespace {
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
}

void Tagger::TagMolecule(OEChem::OEMolBase& mol) {
    if (IsTagged(mol)) {
        return;  // Already tagged
    }

    // TODO: Implement component classification using OESplitMolComplex
    // For now, mark as tagged without classification
    mol.SetData<unsigned int>(GetTaggedTag(), 1);
}

bool Tagger::HasComponent(const OEChem::OEAtomBase& atom, ComponentFlag flag) {
    uint32_t flags = GetFlags(atom);
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
