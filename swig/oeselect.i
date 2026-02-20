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
#include "oeselect/ResidueSelector.h"
#include "oeselect/CustomPredicates.h"

#include <oechem.h>

using namespace OESel;
%}

// ============================================================================
// Forward declarations for cross-module SWIG type resolution
// ============================================================================
namespace OEChem {
    class OEMolBase;
    class OEAtomBase;
}

// ============================================================================
// OEMolBase / OEAtomBase typemaps for cross-module SWIG type resolution
// ============================================================================
// OpenEye's Python bindings register molecule objects as "OEGraphMolWrapper *"
// and "OEMolWrapper *" in the SWIG runtime type table. Our module registers
// "OEChem::OEMolBase *". SWIG's automatic cross-module linking doesn't resolve
// these different type names, so we use runtime type queries to bridge them.
// OpenEye uses SWIG runtime v4; our module uses v5. Since the runtimes are
// separate, SWIG_TypeQuery and SWIG_Python_GetSwigThis cannot access OpenEye
// types. We use Python isinstance for type safety and directly extract the
// void* pointer from the SwigPyObject struct layout (stable across versions).

%{
// Minimal SwigPyObject layout compatible across SWIG runtime versions.
// The actual struct may have more fields, but ptr is always first after
// PyObject_HEAD.
struct _SwigPyObjectCompat {
    PyObject_HEAD
    void *ptr;
};

static PyObject* _oeselect_oe_molbase_type = NULL;
static PyObject* _oeselect_oe_atombase_type = NULL;

static bool _oeselect_is_oemolbase(PyObject* obj) {
    if (!_oeselect_oe_molbase_type) {
        PyObject* mod = PyImport_ImportModule("openeye.oechem");
        if (mod) {
            _oeselect_oe_molbase_type = PyObject_GetAttrString(mod, "OEMolBase");
            Py_DECREF(mod);
        }
        if (!_oeselect_oe_molbase_type) return false;
    }
    return PyObject_IsInstance(obj, _oeselect_oe_molbase_type) == 1;
}

static bool _oeselect_is_oeatombase(PyObject* obj) {
    if (!_oeselect_oe_atombase_type) {
        PyObject* mod = PyImport_ImportModule("openeye.oechem");
        if (mod) {
            _oeselect_oe_atombase_type = PyObject_GetAttrString(mod, "OEAtomBase");
            Py_DECREF(mod);
        }
        if (!_oeselect_oe_atombase_type) return false;
    }
    return PyObject_IsInstance(obj, _oeselect_oe_atombase_type) == 1;
}

static void* _oeselect_extract_swig_ptr(PyObject* obj) {
    // Get the .this attribute which is a SwigPyObject from any SWIG version
    PyObject* thisAttr = PyObject_GetAttrString(obj, "this");
    if (!thisAttr) {
        PyErr_Clear();
        return NULL;
    }
    // Extract the void* ptr from the SwigPyObject-compatible layout
    void* ptr = ((_SwigPyObjectCompat*)thisAttr)->ptr;
    Py_DECREF(thisAttr);
    return ptr;
}

// Helper function to evaluate selection and return matching atom indices
std::vector<unsigned int> EvaluateSelection(
    OEChem::OEMolBase& mol,
    const std::string& selection_str
) {
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
    OEChem::OEMolBase& mol,
    const std::string& selection_str
) {
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

// Typemap for non-const OEMolBase& (needed by OESelect constructor)
%typemap(in) OEChem::OEMolBase& (void *argp = 0, int res = 0) {
    res = SWIG_ConvertPtr($input, &argp, $descriptor, 0);
    if (!SWIG_IsOK(res)) {
        if (_oeselect_is_oemolbase($input)) {
            argp = _oeselect_extract_swig_ptr($input);
            if (argp) res = SWIG_OK;
        }
    }
    if (!SWIG_IsOK(res)) {
        SWIG_exception_fail(SWIG_ArgError(res), "Expected OEMolBase-derived object. Ensure openeye.oechem is imported.");
    }
    if (!argp) {
        SWIG_exception_fail(SWIG_NullReferenceError, "Null OEMolBase reference.");
    }
    $1 = reinterpret_cast< $1_ltype >(argp);
}

%typemap(typecheck, precedence=10) OEChem::OEMolBase& {
    void *vptr = 0;
    int res = SWIG_ConvertPtr($input, &vptr, $descriptor, SWIG_POINTER_NO_NULL);
    $1 = SWIG_IsOK(res) ? 1 : _oeselect_is_oemolbase($input) ? 1 : 0;
}

// Typemap for const OEMolBase& (for utility functions)
%typemap(in) const OEChem::OEMolBase& (void *argp = 0, int res = 0) {
    res = SWIG_ConvertPtr($input, &argp, $descriptor, 0);
    if (!SWIG_IsOK(res)) {
        if (_oeselect_is_oemolbase($input)) {
            argp = _oeselect_extract_swig_ptr($input);
            if (argp) res = SWIG_OK;
        }
    }
    if (!SWIG_IsOK(res)) {
        SWIG_exception_fail(SWIG_ArgError(res), "Expected OEMolBase-derived object. Ensure openeye.oechem is imported.");
    }
    if (!argp) {
        SWIG_exception_fail(SWIG_NullReferenceError, "Null OEMolBase reference.");
    }
    $1 = reinterpret_cast< $1_ltype >(argp);
}

%typemap(typecheck, precedence=10) const OEChem::OEMolBase& {
    void *vptr = 0;
    int res = SWIG_ConvertPtr($input, &vptr, $descriptor, SWIG_POINTER_NO_NULL);
    $1 = SWIG_IsOK(res) ? 1 : _oeselect_is_oemolbase($input) ? 1 : 0;
}

// Typemap for const OEAtomBase& (for OESelect::operator() and predicates)
%typemap(in) const OEChem::OEAtomBase& (void *argp = 0, int res = 0) {
    res = SWIG_ConvertPtr($input, &argp, $descriptor, 0);
    if (!SWIG_IsOK(res)) {
        if (_oeselect_is_oeatombase($input)) {
            argp = _oeselect_extract_swig_ptr($input);
            if (argp) res = SWIG_OK;
        }
    }
    if (!SWIG_IsOK(res)) {
        SWIG_exception_fail(SWIG_ArgError(res), "Expected OEAtomBase-derived object. Ensure openeye.oechem is imported.");
    }
    if (!argp) {
        SWIG_exception_fail(SWIG_NullReferenceError, "Null OEAtomBase reference.");
    }
    $1 = reinterpret_cast< $1_ltype >(argp);
}

%typemap(typecheck, precedence=10) const OEChem::OEAtomBase& {
    void *vptr = 0;
    int res = SWIG_ConvertPtr($input, &vptr, $descriptor, SWIG_POINTER_NO_NULL);
    $1 = SWIG_IsOK(res) ? 1 : _oeselect_is_oeatombase($input) ? 1 : 0;
}

// ============================================================================
// Include STL typemaps
// ============================================================================
%include "std_string.i"
%include "std_vector.i"
%include "std_set.i"
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
// Template instantiations for container types
// ============================================================================
%template(UnsignedIntVector) std::vector<unsigned int>;
%template(SelectorSet) std::set<OESel::Selector>;
%template(StringSet) std::set<std::string>;

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
    ~OESelection();

    std::string ToCanonical() const;
    bool ContainsPredicate(PredicateType type) const;
    bool IsEmpty() const;
};

// ============================================================================
// OESelect - molecule-bound selector
// ============================================================================
class OESelect {
public:
    OESelect(OEChem::OEMolBase& mol, const OESelection& sele);
    OESelect(OEChem::OEMolBase& mol, const std::string& sele);
    OESelect(const OESelect& other);
    ~OESelect();

    bool operator()(const OEChem::OEAtomBase& atom) const;
    const OESelection& GetSelection() const;
};

// ============================================================================
// Selector - residue position identifier
// ============================================================================
struct Selector {
    std::string name;
    int residue_number;
    std::string chain;
    std::string insert_code;

    Selector();
    Selector(std::string name, int residue_number, std::string chain,
             std::string insert_code = " ");

    static Selector FromAtom(const OEChem::OEAtomBase& atom);
    static Selector FromString(const std::string& selector_str);
    std::string ToString() const;

    bool operator<(const Selector& other) const;
    bool operator>(const Selector& other) const;
    bool operator<=(const Selector& other) const;
    bool operator>=(const Selector& other) const;
    bool operator==(const Selector& other) const;
    bool operator!=(const Selector& other) const;
};

// ============================================================================
// OEResidueSelector - predicate matching atoms by residue
// ============================================================================
class OEResidueSelector {
public:
    explicit OEResidueSelector(const std::string& selector_str);
    explicit OEResidueSelector(const std::set<Selector>& selectors);
    OEResidueSelector(const OEResidueSelector& other);
    ~OEResidueSelector();

    bool operator()(const OEChem::OEAtomBase& atom) const;
};

// ============================================================================
// Custom predicates
// ============================================================================
class OEHasResidueName {
public:
    explicit OEHasResidueName(const std::string& residue_name,
                              bool case_sensitive = false,
                              bool whitespace = false);
    OEHasResidueName(const OEHasResidueName& other);
    ~OEHasResidueName();

    bool operator()(const OEChem::OEAtomBase& atom) const;
};

class OEHasAtomNameAdvanced {
public:
    explicit OEHasAtomNameAdvanced(const std::string& atom_name,
                                   bool case_sensitive = false,
                                   bool whitespace = false);
    OEHasAtomNameAdvanced(const OEHasAtomNameAdvanced& other);
    ~OEHasAtomNameAdvanced();

    bool operator()(const OEChem::OEAtomBase& atom) const;
};

// ============================================================================
// Utility functions
// ============================================================================
std::set<Selector> ParseSelectorSet(const std::string& selector_str);
std::set<Selector> MolToSelectorSet(const OEChem::OEMolBase& mol);
std::set<std::string> StrSelectorSet(OEChem::OEMolBase& mol, const std::string& selection_str);
std::string GetSelectorString(const OEChem::OEAtomBase& atom);

} // namespace OESel

// ============================================================================
// Bulk helper functions (outside namespace for simpler Python access)
// ============================================================================
std::vector<unsigned int> EvaluateSelection(
    OEChem::OEMolBase& mol,
    const std::string& selection_str
);

unsigned int CountSelection(
    OEChem::OEMolBase& mol,
    const std::string& selection_str
);

// ============================================================================
// Python extensions for OESelection
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
// Python extensions for OESelect
// ============================================================================
%extend OESel::OESelect {
%pythoncode %{
def __repr__(self):
    return f"OESelect('{self.GetSelection().ToCanonical()}')"
%}
}

// ============================================================================
// Python extensions for Selector
// ============================================================================
%extend OESel::Selector {
%pythoncode %{
def __repr__(self):
    return f"Selector('{self.ToString()}')"

def __str__(self):
    return self.ToString()

def __hash__(self):
    return hash((self.name, self.residue_number, self.chain, self.insert_code))
%}
}

// ============================================================================
// Module-level Python code
// ============================================================================
%pythoncode %{
def select(mol, selection_str):
    """Evaluate a selection string on an OpenEye molecule.

    :param mol: An OpenEye OEMolBase object (OEMol, OEGraphMol, etc.).
    :param selection_str: PyMOL-style selection string (e.g., "name CA", "protein and chain A").
    :returns: List of atom indices that match the selection.

    Example::

        from openeye import oechem
        from oeselect import select

        mol = oechem.OEGraphMol()
        oechem.OESmilesToMol(mol, "CC(=O)OC1=CC=CC=C1C(=O)O")  # Aspirin

        # Select all carbon atoms
        carbon_indices = select(mol, "elem C")

        # Select backbone atoms in chain A
        bb_indices = select(mol, "backbone and chain A")
    """
    return list(EvaluateSelection(mol, selection_str))

def count(mol, selection_str):
    """Count atoms matching a selection in an OpenEye molecule.

    :param mol: An OpenEye OEMolBase object.
    :param selection_str: PyMOL-style selection string.
    :returns: Number of atoms matching the selection.

    Example::

        num_carbons = count(mol, "elem C")
    """
    return CountSelection(mol, selection_str)

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

def str_selector_set(mol, selection_str):
    """Extract unique residue selector strings for atoms matching a selection.

    :param mol: An OpenEye OEMolBase object.
    :param selection_str: PyMOL-style selection string.
    :returns: Set of unique selector strings in "NAME:NUMBER:ICODE:CHAIN" format.

    Example::

        selectors = str_selector_set(mol, "protein")
    """
    return StrSelectorSet(mol, selection_str)

def selector_set(selector_str):
    """Parse a selector string into a set of Selector objects.

    :param selector_str: Comma/semicolon/newline-separated selector strings.
    :returns: Set of Selector objects.

    Example::

        sels = selector_set("ALA:123: :A,GLY:124: :A")
    """
    return ParseSelectorSet(selector_str)

def mol_to_selector_set(mol):
    """Extract unique Selector objects from all atoms in a molecule.

    :param mol: An OpenEye OEMolBase object.
    :returns: Set of Selector objects for all unique residues.

    Example::

        selectors = mol_to_selector_set(mol)
    """
    return MolToSelectorSet(mol)

def get_selector_string(atom):
    """Get the selector string for an atom.

    :param atom: An OpenEye OEAtomBase object.
    :returns: Selector string in "NAME:NUMBER:ICODE:CHAIN" format.

    Example::

        for atom in mol.GetAtoms():
            print(get_selector_string(atom))
    """
    return GetSelectorString(atom)

__version__ = "1.0.0"
%}
