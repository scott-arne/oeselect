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
#include <oegrid.h>

using namespace OESel;
%}

// ============================================================================
// Forward declarations for cross-module SWIG type resolution
// ============================================================================
// These enable typemaps for OpenEye types whose definitions live in the
// OpenEye SWIG runtime (v4). Only types you actually use in your wrapped API
// need full #include — forward declarations suffice for the typemaps.

namespace OEChem {
    class OEMolBase;
    class OEMCMolBase;
    class OEMol;
    class OEGraphMol;
    class OEAtomBase;
    class OEBondBase;
    class OEConfBase;
    class OEMatchBase;
    class OEMolDatabase;
    class oemolistream;
    class oemolostream;
    class OEQMol;
    class OEResidue;
    class OEUniMolecularRxn;
}

namespace OEBio {
    class OEDesignUnit;
    class OEHierView;
    class OEHierResidue;
    class OEHierFragment;
    class OEHierChain;
    class OEInteractionHint;
    class OEInteractionHintContainer;
}

namespace OEDocking {
    class OEReceptor;
}

namespace OEPlatform {
    class oeifstream;
    class oeofstream;
    class oeisstream;
    class oeosstream;
}

namespace OESystem {
    class OEScalarGrid;
    class OERecord;
    class OEMolRecord;
}

// ============================================================================
// Cross-runtime SWIG compatibility layer
// ============================================================================
// OpenEye's Python bindings use SWIG runtime v4; our module uses v5.
// Since the runtimes are separate, SWIG_TypeQuery cannot access OpenEye types.
// We use Python isinstance for type safety and directly extract the void*
// pointer from the SwigPyObject struct layout (stable across SWIG versions).
//
// This approach enables passing OpenEye objects between Python and C++ without
// serialization. The macros below generate the boilerplate for each type.

%{
// Minimal SwigPyObject layout compatible across SWIG runtime versions.
// The actual struct may have more fields, but ptr is always first after
// PyObject_HEAD.
struct _SwigPyObjectCompat {
    PyObject_HEAD
    void *ptr;
};

static void* _oeselect_extract_swig_ptr(PyObject* obj) {
    PyObject* thisAttr = PyObject_GetAttrString(obj, "this");
    if (!thisAttr) {
        PyErr_Clear();
        return NULL;
    }
    void* ptr = ((_SwigPyObjectCompat*)thisAttr)->ptr;
    Py_DECREF(thisAttr);
    return ptr;
}

// ---- Type checker generator macro ----
// Generates a cached isinstance checker for an OpenEye Python type.
// TAG:    identifier suffix (e.g., oemolbase)
// MODULE: Python module string (e.g., "openeye.oechem")
// CLASS:  Python class name string (e.g., "OEMolBase")
#define DEFINE_OE_TYPE_CHECKER(TAG, MODULE, CLASS) \
    static PyObject* _oeselect_oe_##TAG##_type = NULL; \
    static bool _oeselect_is_##TAG(PyObject* obj) { \
        if (!_oeselect_oe_##TAG##_type) { \
            PyObject* mod = PyImport_ImportModule(MODULE); \
            if (mod) { \
                _oeselect_oe_##TAG##_type = PyObject_GetAttrString(mod, CLASS); \
                Py_DECREF(mod); \
            } \
            if (!_oeselect_oe_##TAG##_type) return false; \
        } \
        return PyObject_IsInstance(obj, _oeselect_oe_##TAG##_type) == 1; \
    }

// ---- Molecule types (openeye.oechem) ----
DEFINE_OE_TYPE_CHECKER(oemolbase,    "openeye.oechem", "OEMolBase")
DEFINE_OE_TYPE_CHECKER(oemcmolbase,  "openeye.oechem", "OEMCMolBase")
DEFINE_OE_TYPE_CHECKER(oemol,        "openeye.oechem", "OEMol")
DEFINE_OE_TYPE_CHECKER(oegraphmol,   "openeye.oechem", "OEGraphMol")
DEFINE_OE_TYPE_CHECKER(oeqmol,       "openeye.oechem", "OEQMol")

// ---- Atom / bond / conformer / residue (openeye.oechem) ----
DEFINE_OE_TYPE_CHECKER(oeatombase,   "openeye.oechem", "OEAtomBase")
DEFINE_OE_TYPE_CHECKER(oebondbase,   "openeye.oechem", "OEBondBase")
DEFINE_OE_TYPE_CHECKER(oeconfbase,   "openeye.oechem", "OEConfBase")
DEFINE_OE_TYPE_CHECKER(oeresidue,    "openeye.oechem", "OEResidue")
DEFINE_OE_TYPE_CHECKER(oematchbase,  "openeye.oechem", "OEMatchBase")

// ---- Molecule I/O (openeye.oechem) ----
DEFINE_OE_TYPE_CHECKER(oemolistream, "openeye.oechem", "oemolistream")
DEFINE_OE_TYPE_CHECKER(oemolostream, "openeye.oechem", "oemolostream")
DEFINE_OE_TYPE_CHECKER(oemoldatabase,"openeye.oechem", "OEMolDatabase")

// ---- Reactions (openeye.oechem) ----
DEFINE_OE_TYPE_CHECKER(oeunimolecularrxn, "openeye.oechem", "OEUniMolecularRxn")

// ---- Platform streams (openeye.oechem) ----
DEFINE_OE_TYPE_CHECKER(oeifstream,   "openeye.oechem", "oeifstream")
DEFINE_OE_TYPE_CHECKER(oeofstream,   "openeye.oechem", "oeofstream")
DEFINE_OE_TYPE_CHECKER(oeisstream,   "openeye.oechem", "oeisstream")
DEFINE_OE_TYPE_CHECKER(oeosstream,   "openeye.oechem", "oeosstream")

// ---- Records (openeye.oechem) ----
DEFINE_OE_TYPE_CHECKER(oerecord,     "openeye.oechem", "OERecord")
DEFINE_OE_TYPE_CHECKER(oemolrecord,  "openeye.oechem", "OEMolRecord")

// ---- Bio / hierarchy (openeye.oechem) ----
DEFINE_OE_TYPE_CHECKER(oedesignunit, "openeye.oechem", "OEDesignUnit")
DEFINE_OE_TYPE_CHECKER(oehierview,   "openeye.oechem", "OEHierView")
DEFINE_OE_TYPE_CHECKER(oehierresidue,"openeye.oechem", "OEHierResidue")
DEFINE_OE_TYPE_CHECKER(oehierfragment,"openeye.oechem","OEHierFragment")
DEFINE_OE_TYPE_CHECKER(oehierchain,  "openeye.oechem", "OEHierChain")
DEFINE_OE_TYPE_CHECKER(oeinteractionhint,          "openeye.oechem", "OEInteractionHint")
DEFINE_OE_TYPE_CHECKER(oeinteractionhintcontainer, "openeye.oechem", "OEInteractionHintContainer")

// ---- Grid (openeye.oegrid) ----
DEFINE_OE_TYPE_CHECKER(oescalargrid, "openeye.oegrid", "OEScalarGrid")

// ---- Docking (openeye.oedocking) ----
DEFINE_OE_TYPE_CHECKER(oereceptor,   "openeye.oedocking", "OEReceptor")

#undef DEFINE_OE_TYPE_CHECKER

// ---- OEScalarGrid return-type helper (zero-copy pointer swap) ----
static PyObject* _oeselect_wrap_as_oe_grid(OESystem::OEScalarGrid* grid) {
    if (!grid) {
        Py_RETURN_NONE;
    }
    PyObject* oegrid_mod = PyImport_ImportModule("openeye.oegrid");
    if (!oegrid_mod) {
        delete grid;
        return NULL;
    }
    PyObject* grid_cls = PyObject_GetAttrString(oegrid_mod, "OEScalarGrid");
    Py_DECREF(oegrid_mod);
    if (!grid_cls) {
        delete grid;
        return NULL;
    }
    PyObject* oe_grid = PyObject_CallNoArgs(grid_cls);
    Py_DECREF(grid_cls);
    if (!oe_grid) {
        delete grid;
        return NULL;
    }
    PyObject* thisAttr = PyObject_GetAttrString(oe_grid, "this");
    if (!thisAttr) {
        PyErr_Clear();
        Py_DECREF(oe_grid);
        delete grid;
        return NULL;
    }
    _SwigPyObjectCompat* swig_this = (_SwigPyObjectCompat*)thisAttr;
    delete reinterpret_cast<OESystem::OEScalarGrid*>(swig_this->ptr);
    swig_this->ptr = grid;
    Py_DECREF(thisAttr);
    return oe_grid;
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

// ============================================================================
// Typemap generator macros
// ============================================================================

// Generate const-ref and non-const-ref typemaps for a cross-runtime OpenEye type.
// CPP_TYPE: fully qualified C++ type (e.g., OEChem::OEMolBase)
// CHECKER:  isinstance checker function name
// ERR_MSG:  error message on type mismatch
%define OE_CROSS_RUNTIME_REF_TYPEMAPS(CPP_TYPE, CHECKER, ERR_MSG)

%typemap(in) const CPP_TYPE& (void *argp = 0, int res = 0) {
    res = SWIG_ConvertPtr($input, &argp, $descriptor, 0);
    if (!SWIG_IsOK(res)) {
        if (CHECKER($input)) {
            argp = _oeselect_extract_swig_ptr($input);
            if (argp) res = SWIG_OK;
        }
    }
    if (!SWIG_IsOK(res)) {
        SWIG_exception_fail(SWIG_ArgError(res), ERR_MSG);
    }
    if (!argp) {
        SWIG_exception_fail(SWIG_NullReferenceError, "Null reference.");
    }
    $1 = reinterpret_cast< $1_ltype >(argp);
}

%typemap(typecheck, precedence=10) const CPP_TYPE& {
    void *vptr = 0;
    int res = SWIG_ConvertPtr($input, &vptr, $descriptor, SWIG_POINTER_NO_NULL);
    $1 = SWIG_IsOK(res) ? 1 : CHECKER($input) ? 1 : 0;
}

%typemap(in) CPP_TYPE& (void *argp = 0, int res = 0) {
    res = SWIG_ConvertPtr($input, &argp, $descriptor, 0);
    if (!SWIG_IsOK(res)) {
        if (CHECKER($input)) {
            argp = _oeselect_extract_swig_ptr($input);
            if (argp) res = SWIG_OK;
        }
    }
    if (!SWIG_IsOK(res)) {
        SWIG_exception_fail(SWIG_ArgError(res), ERR_MSG);
    }
    if (!argp) {
        SWIG_exception_fail(SWIG_NullReferenceError, "Null reference.");
    }
    $1 = reinterpret_cast< $1_ltype >(argp);
}

%typemap(typecheck, precedence=10) CPP_TYPE& {
    void *vptr = 0;
    int res = SWIG_ConvertPtr($input, &vptr, $descriptor, SWIG_POINTER_NO_NULL);
    $1 = SWIG_IsOK(res) ? 1 : CHECKER($input) ? 1 : 0;
}

%enddef

// Generate nullable-pointer typemaps (accepts None) for a cross-runtime type.
%define OE_CROSS_RUNTIME_NULLABLE_PTR_TYPEMAPS(CPP_TYPE, CHECKER, ERR_MSG)

%typemap(in) const CPP_TYPE* (void *argp = 0, int res = 0) {
    if ($input == Py_None) {
        $1 = NULL;
    } else {
        res = SWIG_ConvertPtr($input, &argp, $descriptor, 0);
        if (!SWIG_IsOK(res)) {
            if (CHECKER($input)) {
                argp = _oeselect_extract_swig_ptr($input);
                if (argp) res = SWIG_OK;
            }
        }
        if (!SWIG_IsOK(res)) {
            SWIG_exception_fail(SWIG_ArgError(res), ERR_MSG);
        }
        $1 = reinterpret_cast< $1_ltype >(argp);
    }
}

%typemap(typecheck, precedence=10) const CPP_TYPE* {
    if ($input == Py_None) {
        $1 = 1;
    } else {
        void *vptr = 0;
        int res = SWIG_ConvertPtr($input, &vptr, $descriptor, 0);
        $1 = SWIG_IsOK(res) ? 1 : CHECKER($input) ? 1 : 0;
    }
}

%enddef

// ============================================================================
// Typemap declarations for all OpenEye types
// ============================================================================
// Each type gets const-ref and non-const-ref typemaps. Types that commonly
// appear as optional parameters also get nullable-pointer typemaps.
// These are inert until a wrapped function signature uses the type.

// ---- Molecule hierarchy (OEChem) ----
OE_CROSS_RUNTIME_REF_TYPEMAPS(OEChem::OEMolBase,    _oeselect_is_oemolbase,    "Expected OEMolBase-derived object.")
OE_CROSS_RUNTIME_REF_TYPEMAPS(OEChem::OEMCMolBase,  _oeselect_is_oemcmolbase,  "Expected OEMCMolBase-derived object (OEMCMolBase or OEMol).")
OE_CROSS_RUNTIME_REF_TYPEMAPS(OEChem::OEMol,        _oeselect_is_oemol,        "Expected OEMol object.")
OE_CROSS_RUNTIME_REF_TYPEMAPS(OEChem::OEGraphMol,   _oeselect_is_oegraphmol,   "Expected OEGraphMol object.")
OE_CROSS_RUNTIME_REF_TYPEMAPS(OEChem::OEQMol,       _oeselect_is_oeqmol,       "Expected OEQMol object.")

// ---- Atom / bond / conformer / residue / match (OEChem) ----
OE_CROSS_RUNTIME_REF_TYPEMAPS(OEChem::OEAtomBase,   _oeselect_is_oeatombase,   "Expected OEAtomBase-derived object.")
OE_CROSS_RUNTIME_REF_TYPEMAPS(OEChem::OEBondBase,   _oeselect_is_oebondbase,   "Expected OEBondBase-derived object.")
OE_CROSS_RUNTIME_REF_TYPEMAPS(OEChem::OEConfBase,   _oeselect_is_oeconfbase,   "Expected OEConfBase-derived object.")
OE_CROSS_RUNTIME_REF_TYPEMAPS(OEChem::OEResidue,    _oeselect_is_oeresidue,    "Expected OEResidue object.")
OE_CROSS_RUNTIME_REF_TYPEMAPS(OEChem::OEMatchBase,  _oeselect_is_oematchbase,  "Expected OEMatchBase-derived object.")

// ---- Molecule I/O (OEChem) ----
OE_CROSS_RUNTIME_REF_TYPEMAPS(OEChem::oemolistream,  _oeselect_is_oemolistream, "Expected oemolistream object.")
OE_CROSS_RUNTIME_REF_TYPEMAPS(OEChem::oemolostream,  _oeselect_is_oemolostream, "Expected oemolostream object.")
OE_CROSS_RUNTIME_REF_TYPEMAPS(OEChem::OEMolDatabase, _oeselect_is_oemoldatabase,"Expected OEMolDatabase object.")

// ---- Reactions (OEChem) ----
OE_CROSS_RUNTIME_REF_TYPEMAPS(OEChem::OEUniMolecularRxn, _oeselect_is_oeunimolecularrxn, "Expected OEUniMolecularRxn object.")

// ---- Platform streams (OEPlatform) ----
OE_CROSS_RUNTIME_REF_TYPEMAPS(OEPlatform::oeifstream, _oeselect_is_oeifstream, "Expected oeifstream object.")
OE_CROSS_RUNTIME_REF_TYPEMAPS(OEPlatform::oeofstream, _oeselect_is_oeofstream, "Expected oeofstream object.")
OE_CROSS_RUNTIME_REF_TYPEMAPS(OEPlatform::oeisstream, _oeselect_is_oeisstream, "Expected oeisstream object.")
OE_CROSS_RUNTIME_REF_TYPEMAPS(OEPlatform::oeosstream, _oeselect_is_oeosstream, "Expected oeosstream object.")

// ---- Records (OESystem) ----
OE_CROSS_RUNTIME_REF_TYPEMAPS(OESystem::OERecord,    _oeselect_is_oerecord,    "Expected OERecord object.")
OE_CROSS_RUNTIME_REF_TYPEMAPS(OESystem::OEMolRecord, _oeselect_is_oemolrecord, "Expected OEMolRecord object.")

// ---- Bio / hierarchy (OEBio) ----
OE_CROSS_RUNTIME_REF_TYPEMAPS(OEBio::OEDesignUnit,   _oeselect_is_oedesignunit, "Expected OEDesignUnit object.")
OE_CROSS_RUNTIME_REF_TYPEMAPS(OEBio::OEHierView,     _oeselect_is_oehierview,   "Expected OEHierView object.")
OE_CROSS_RUNTIME_REF_TYPEMAPS(OEBio::OEHierResidue,  _oeselect_is_oehierresidue,"Expected OEHierResidue object.")
OE_CROSS_RUNTIME_REF_TYPEMAPS(OEBio::OEHierFragment,  _oeselect_is_oehierfragment,"Expected OEHierFragment object.")
OE_CROSS_RUNTIME_REF_TYPEMAPS(OEBio::OEHierChain,    _oeselect_is_oehierchain,  "Expected OEHierChain object.")
OE_CROSS_RUNTIME_REF_TYPEMAPS(OEBio::OEInteractionHint,          _oeselect_is_oeinteractionhint,          "Expected OEInteractionHint object.")
OE_CROSS_RUNTIME_REF_TYPEMAPS(OEBio::OEInteractionHintContainer, _oeselect_is_oeinteractionhintcontainer, "Expected OEInteractionHintContainer object.")

// ---- Grid (OESystem) ----
OE_CROSS_RUNTIME_REF_TYPEMAPS(OESystem::OEScalarGrid, _oeselect_is_oescalargrid, "Expected OEScalarGrid-derived object.")
OE_CROSS_RUNTIME_NULLABLE_PTR_TYPEMAPS(OESystem::OEScalarGrid, _oeselect_is_oescalargrid, "Expected OEScalarGrid or None.")

// OEScalarGrid return-type typemap (wraps C++ grid as native openeye.oegrid object)
%typemap(out) OESystem::OEScalarGrid* {
    $result = _oeselect_wrap_as_oe_grid($1);
    if (!$result) SWIG_fail;
}

// ---- Docking (OEDocking) ----
OE_CROSS_RUNTIME_REF_TYPEMAPS(OEDocking::OEReceptor, _oeselect_is_oereceptor, "Expected OEReceptor object.")

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
#define OESELECT_VERSION_MINOR 3
#define OESELECT_VERSION_PATCH 3

// ============================================================================
// PredicateType enum
// ============================================================================
namespace OESel {

enum class PredicateType {
    AND, OR, NOT, XOR,
    NAME, RESN, RESI, CHAIN, ELEM, INDEX, ID, ALT, B_FACTOR, FRAGMENT,
    SECONDARY_STRUCTURE,
    PROTEIN, LIGAND, WATER, SOLVENT, ORGANIC, BACKBONE, METAL, CAPPING,
    HEAVY, HYDROGEN, POLAR_HYDROGEN, NONPOLAR_HYDROGEN,
    BY_RES, BY_CHAIN,
    AROUND, EXPAND, BEYOND,
    HELIX, SHEET, TURN, LOOP,
    ALL_MATCH, NO_MATCH
};

// ============================================================================
// SelectionError exception
// ============================================================================
class SelectionError : public std::runtime_error {
public:
    explicit SelectionError(const std::string& message);
    SelectionError(const std::string& message, size_t position);
    size_t Position() const;
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
std::set<Selector> parse_selector_set(const std::string& selector_str);
std::set<Selector> mol_to_selector_set(const OEChem::OEMolBase& mol);
std::set<std::string> str_selector_set(OEChem::OEMolBase& mol, const std::string& selection_str);
std::string get_selector_string(const OEChem::OEAtomBase& atom);

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
    return _oeselect.str_selector_set(mol, selection_str)

def selector_set(selector_str):
    """Parse a selector string into a set of Selector objects.

    :param selector_str: Comma/semicolon/newline-separated selector strings.
    :returns: Set of Selector objects.

    Example::

        sels = selector_set("ALA:123: :A,GLY:124: :A")
    """
    return _oeselect.parse_selector_set(selector_str)

def mol_to_selector_set(mol):
    """Extract unique Selector objects from all atoms in a molecule.

    :param mol: An OpenEye OEMolBase object.
    :returns: Set of Selector objects for all unique residues.

    Example::

        selectors = mol_to_selector_set(mol)
    """
    return _oeselect.mol_to_selector_set(mol)

def get_selector_string(atom):
    """Get the selector string for an atom.

    :param atom: An OpenEye OEAtomBase object.
    :returns: Selector string in "NAME:NUMBER:ICODE:CHAIN" format.

    Example::

        for atom in mol.GetAtoms():
            print(get_selector_string(atom))
    """
    return _oeselect.get_selector_string(atom)

__version__ = "1.1.3"
%}
