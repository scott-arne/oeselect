// swig/oeselect.i
// SWIG interface file for OESelect Python bindings
%module _oeselect

%{
// Include all necessary headers
#include "oeselect/oeselect.h"
#include "oeselect/Selection.h"
#include "oeselect/Selector.h"
#include "oeselect/Context.h"
#include "oeselect/Predicate.h"
#include "oeselect/Error.h"

#include <oechem.h>

using namespace OESel;

// Helper function to evaluate selection on a molecule and return matching atom indices
std::vector<unsigned int> EvaluateSelection(
    const std::string& selection_str,
    const std::string& smiles
) {
    OEChem::OEGraphMol mol;
    if (!OEChem::OESmilesToMol(mol, smiles)) {
        throw OESel::SelectionError("Failed to parse SMILES: " + smiles);
    }

    OESel::OESelection sele = OESel::OESelection::Parse(selection_str);
    OESel::OESelect selector(mol, sele);

    std::vector<unsigned int> result;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
        if (selector(*atom)) {
            result.push_back(atom->GetIdx());
        }
    }
    return result;
}

// Helper function to count atoms matching a selection
unsigned int CountSelection(
    const std::string& selection_str,
    const std::string& smiles
) {
    OEChem::OEGraphMol mol;
    if (!OEChem::OESmilesToMol(mol, smiles)) {
        throw OESel::SelectionError("Failed to parse SMILES: " + smiles);
    }

    OESel::OESelection sele = OESel::OESelection::Parse(selection_str);
    OESel::OESelect selector(mol, sele);

    unsigned int count = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
        if (selector(*atom)) {
            count++;
        }
    }
    return count;
}
%}

// ============================================================================
// Include STL typemaps
// ============================================================================
%include "std_string.i"
%include "std_vector.i"
%include "stdint.i"
%include "exception.i"

// ============================================================================
// Exception handling
// ============================================================================
%exception {
    try {
        $action
    } catch (const OESel::SelectionError& e) {
        SWIG_exception(SWIG_ValueError, e.what());
    } catch (const std::exception& e) {
        SWIG_exception(SWIG_RuntimeError, e.what());
    } catch (...) {
        SWIG_exception(SWIG_RuntimeError, "Unknown C++ exception");
    }
}

// ============================================================================
// Template instantiations for vector types
// ============================================================================
%template(UnsignedIntVector) std::vector<unsigned int>;

// ============================================================================
// Version macros
// ============================================================================
#define OESELECT_VERSION_MAJOR 1
#define OESELECT_VERSION_MINOR 0
#define OESELECT_VERSION_PATCH 0

// ============================================================================
// PredicateType enum
// ============================================================================
namespace OESel {

enum class PredicateType {
    And, Or, Not, XOr,
    Name, Resn, Resi, Chain, Elem, Index, SecondaryStructure,
    Protein, Ligand, Water, Solvent, Organic, Backbone, Metal,
    Heavy, Hydrogen, PolarHydrogen, NonpolarHydrogen,
    ByRes, ByChain,
    Around, XAround, Box, XBox, Beyond,
    Helix, Sheet, Turn, Loop,
    True, False
};

// ============================================================================
// SelectionError exception
// ============================================================================
class SelectionError : public std::runtime_error {
public:
    explicit SelectionError(const std::string& message);
    SelectionError(const std::string& message, size_t position);
    size_t position() const;
};

// ============================================================================
// OESelection - immutable parsed selection
// ============================================================================
class OESelection {
public:
    static OESelection Parse(const std::string& sele);

    OESelection();
    OESelection(const OESelection& other);
    OESelection(OESelection&& other) noexcept;
    ~OESelection();

    OESelection& operator=(const OESelection& other);
    OESelection& operator=(OESelection&& other) noexcept;

    std::string ToCanonical() const;
    bool ContainsPredicate(PredicateType type) const;
    bool IsEmpty() const;
};

} // namespace OESel

// ============================================================================
// Helper functions for Python
// ============================================================================
std::vector<unsigned int> EvaluateSelection(
    const std::string& selection_str,
    const std::string& smiles
);

unsigned int CountSelection(
    const std::string& selection_str,
    const std::string& smiles
);

// ============================================================================
// Python extensions
// ============================================================================
%extend OESel::OESelection {
%pythoncode %{
def __repr__(self):
    return f"OESelection('{self.ToCanonical()}')"

def __str__(self):
    return self.ToCanonical()
%}
}

// ============================================================================
// Module-level Python code
// ============================================================================
%pythoncode %{
def select(selection_str, mol):
    """Evaluate a selection string on an OpenEye molecule.

    :param selection_str: PyMOL-style selection string (e.g., "name CA", "protein and chain A").
    :param mol: An OpenEye OEMolBase object (OEMol, OEGraphMol, etc.).
    :returns: List of atom indices that match the selection.

    Example::

        from openeye import oechem
        from oeselect_lib import select

        mol = oechem.OEGraphMol()
        oechem.OESmilesToMol(mol, "CC(=O)OC1=CC=CC=C1C(=O)O")  # Aspirin

        # Select all carbon atoms
        carbon_indices = select("elem C", mol)

        # Select backbone atoms in chain A
        bb_indices = select("backbone and chain A", mol)
    """
    from openeye import oechem
    smiles = oechem.OEMolToSmiles(mol)
    return list(EvaluateSelection(selection_str, smiles))

def count(selection_str, mol):
    """Count atoms matching a selection in an OpenEye molecule.

    :param selection_str: PyMOL-style selection string.
    :param mol: An OpenEye OEMolBase object.
    :returns: Number of atoms matching the selection.

    Example::

        num_carbons = count("elem C", mol)
    """
    from openeye import oechem
    smiles = oechem.OEMolToSmiles(mol)
    return CountSelection(selection_str, smiles)

def parse(selection_str):
    """Parse a selection string and return an OESelection object.

    :param selection_str: PyMOL-style selection string.
    :returns: OESelection object that can be used for validation and canonicalization.
    :raises ValueError: If the selection string is invalid.

    Example::

        sele = parse("name CA and protein")
        print(sele.ToCanonical())  # Normalized form
    """
    return OESelection.Parse(selection_str)

__version__ = "1.0.0"
%}
