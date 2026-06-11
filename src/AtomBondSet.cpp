/**
 * @file AtomBondSet.cpp
 * @brief atom_bond_set implementation.
 */

#include "oeselect/AtomBondSet.h"

#include <vector>

#include "oeselect/Selection.h"
#include "oeselect/Selector.h"

#include <oechem.h>

namespace OESel {

OEChem::OEAtomBondSet atom_bond_set(const OESelect& selector) {
    // OEAtomBondSet stores mutable pointers, so iterate the molecule mutably.
    OEChem::OEMolBase& mol = const_cast<OEChem::OEMolBase&>(selector.GetMol());

    OEChem::OEAtomBondSet result;

    // Pass 1: record selected atoms in an index-keyed mask and add them.
    // GetMaxAtomIdx() bounds GetIdx() and tolerates gaps from deleted atoms.
    std::vector<bool> selected(mol.GetMaxAtomIdx(), false);
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
        if (selector(*atom)) {
            selected[atom->GetIdx()] = true;
            result.AddAtom(&*atom);
        }
    }

    // Pass 2: add bonds whose begin and end atoms are both selected.
    for (OESystem::OEIter<OEChem::OEBondBase> bond = mol.GetBonds(); bond; ++bond) {
        const unsigned int bgn = bond->GetBgn()->GetIdx();
        const unsigned int end = bond->GetEnd()->GetIdx();
        // Bounds check is defensive: indices from this molecule are always in
        // range; the guard protects against malformed graphs.
        if (bgn < selected.size() && end < selected.size() &&
            selected[bgn] && selected[end]) {
            result.AddBond(&*bond);
        }
    }

    return result;
}

OEChem::OEAtomBondSet atom_bond_set(
        OEChem::OEMolBase& mol, const OESelection& selection) {
    const OESelect selector(mol, selection);
    return atom_bond_set(selector);
}

OEChem::OEAtomBondSet atom_bond_set(
        OEChem::OEMolBase& mol, const std::string& selection_str) {
    const OESelection selection = OESelection::Parse(selection_str);
    return atom_bond_set(mol, selection);
}

}  // namespace OESel
