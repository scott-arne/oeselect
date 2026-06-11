/**
 * @file AtomBondSet.h
 * @brief Materialize a selection into an OpenEye OEAtomBondSet.
 *
 * Provides free functions that evaluate a selection against a molecule and
 * return an OEAtomBondSet containing every selected atom plus every bond whose
 * begin and end atoms are both selected.
 */

#ifndef OESELECT_ATOMBONDSET_H
#define OESELECT_ATOMBONDSET_H

#include <string>

#include <oechem.h>

namespace OESel {

class OESelection;
class OESelect;

/**
 * @brief Build an OEAtomBondSet from a parsed selection.
 *
 * The result contains every atom matched by @p selection and every bond whose
 * begin and end atoms are both matched. Bonds with only one selected endpoint
 * are excluded. Runs in O(atoms + bonds).
 *
 * @param mol The molecule to evaluate (must outlive the returned set, which
 *            holds atom and bond pointers into it).
 * @param selection Parsed PyMOL-style selection.
 * @return OEAtomBondSet of selected atoms and fully-internal bonds.
 */
OEChem::OEAtomBondSet atom_bond_set(
    OEChem::OEMolBase& mol, const OESelection& selection);

/**
 * @brief Build an OEAtomBondSet from a selection string.
 *
 * @param mol The molecule to evaluate (must outlive the returned set).
 * @param selection_str PyMOL-style selection string.
 * @return OEAtomBondSet of selected atoms and fully-internal bonds.
 * @throws SelectionError if the selection string cannot be parsed.
 */
OEChem::OEAtomBondSet atom_bond_set(
    OEChem::OEMolBase& mol, const std::string& selection_str);

/**
 * @brief Build an OEAtomBondSet from a molecule-bound selector.
 *
 * Uses the molecule and parsed selection already bound to @p selector.
 *
 * @param selector Molecule-bound selector to evaluate.
 * @return OEAtomBondSet of selected atoms and fully-internal bonds.
 */
OEChem::OEAtomBondSet atom_bond_set(const OESelect& selector);

}  // namespace OESel

#endif  // OESELECT_ATOMBONDSET_H
