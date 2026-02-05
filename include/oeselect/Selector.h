/**
 * @file Selector.h
 * @brief Molecule-bound selector compatible with OpenEye predicates.
 *
 * OESelect binds a selection to a molecule and provides an OpenEye-compatible
 * predicate interface for filtering atoms.
 */

#ifndef OESELECT_SELECTOR_H
#define OESELECT_SELECTOR_H

#include <memory>
#include <string>

#include <oechem.h>

#include "oeselect/Selection.h"

namespace OESel {

/**
 * @brief Molecule-bound selection evaluator.
 *
 * OESelect combines an OESelection with a molecule to create an evaluator
 * that can be used directly with OpenEye's GetAtoms() method. It inherits
 * from OEUnaryPredicate to provide full compatibility.
 *
 * @code
 * // Using with parsed selection
 * auto sele = OESelection::Parse("protein and chain A");
 * OESelect sel(mol, sele);
 * for (auto& atom : mol.GetAtoms(sel)) {
 *     // Process matching atoms
 * }
 *
 * // Direct string construction
 * OESelect sel2(mol, "name CA");
 * unsigned int count = 0;
 * for (auto& atom : mol.GetAtoms(sel2)) {
 *     count++;
 * }
 * @endcode
 *
 * @note The selector maintains internal caches for efficient repeated
 *       evaluation. For best performance, reuse the same OESelect instance.
 */
class OESelect : public OESystem::OEUnaryPredicate<OEChem::OEAtomBase> {
public:
    /**
     * @brief Construct from molecule and parsed selection.
     *
     * @param mol The molecule to select from (must outlive this selector).
     * @param sele The parsed selection to apply.
     */
    OESelect(OEChem::OEMolBase& mol, const OESelection& sele);

    /**
     * @brief Construct from molecule and selection string.
     *
     * Convenience constructor that parses the string internally.
     *
     * @param mol The molecule to select from (must outlive this selector).
     * @param sele PyMOL-style selection string.
     * @throws SelectionError if parsing fails.
     */
    OESelect(OEChem::OEMolBase& mol, const std::string& sele);

    /// @brief Copy constructor (creates new evaluation context).
    OESelect(const OESelect& other);

    /// @brief Copy assignment operator.
    OESelect& operator=(const OESelect& other);

    /// @brief Destructor.
    ~OESelect() override;

    /**
     * @brief Evaluate predicate for an atom.
     *
     * Implements the OEUnaryPredicate interface. Returns true if the
     * atom matches the selection criteria.
     *
     * @param atom The atom to evaluate.
     * @return true if the atom matches the selection.
     */
    bool operator()(const OEChem::OEAtomBase& atom) const override;

    /**
     * @brief Create a copy for OpenEye compatibility.
     *
     * Required by OEUnaryFunction interface. Returns a dynamically
     * allocated copy that the caller must delete.
     *
     * @return New OESelect instance (caller takes ownership).
     */
    OESystem::OEUnaryFunction<OEChem::OEAtomBase, bool>* CreateCopy() const override;

    /**
     * @brief Access the underlying selection.
     *
     * @return Reference to the OESelection used by this selector.
     */
    const OESelection& GetSelection() const;

private:
    struct Impl;
    std::unique_ptr<Impl> pimpl_;  ///< PIMPL for binary compatibility
};

}  // namespace OESel

#endif  // OESELECT_SELECTOR_H
