/**
 * @file ResidueSelector.h
 * @brief Residue selector types and utility functions.
 *
 * Provides a Selector struct for identifying unique residue positions,
 * an OEResidueSelector predicate for matching atoms by residue, and
 * utility functions for extracting and parsing selector sets.
 */

#ifndef OESELECT_RESIDUESELECTOR_H
#define OESELECT_RESIDUESELECTOR_H

#include <set>
#include <string>

#include <oechem.h>

namespace OESel {

/**
 * @brief Identifies a unique residue position in a molecular structure.
 *
 * A Selector combines residue name, number, chain ID, and insert code
 * to uniquely identify a residue. The string format is "NAME:NUMBER:ICODE:CHAIN",
 * e.g., "ALA:123: :A".
 *
 * @code
 * auto sel = Selector::FromString("ALA:123: :A");
 * std::cout << sel.ToString() << "\n";  // "ALA:123: :A"
 *
 * // Extract from an atom
 * auto sel2 = Selector::FromAtom(atom);
 * @endcode
 */
struct Selector {
    std::string name;           ///< Residue name (e.g., "ALA", "GLY")
    int residue_number;         ///< Residue sequence number
    std::string chain;          ///< Chain identifier (e.g., "A", "B")
    std::string insert_code;    ///< PDB insertion code (default " ")

    Selector() : residue_number(0), insert_code(" ") {}

    Selector(std::string name, int residue_number, std::string chain,
             std::string insert_code = " ")
        : name(std::move(name)), residue_number(residue_number),
          chain(std::move(chain)), insert_code(std::move(insert_code)) {}

    /**
     * @brief Create a Selector from an atom's residue information.
     * @param atom The atom to extract residue info from.
     * @return Selector for the atom's residue.
     */
    static Selector FromAtom(const OEChem::OEAtomBase& atom);

    /**
     * @brief Create a Selector from an OEResidue.
     * @param res The residue to extract info from.
     * @return Selector for the residue.
     */
    static Selector FromResidue(const OEChem::OEResidue& res);

    /**
     * @brief Parse a Selector from string format "NAME:NUMBER:ICODE:CHAIN".
     * @param selector_str The selector string to parse.
     * @return Parsed Selector.
     * @throws SelectionError if the format is invalid.
     */
    static Selector FromString(const std::string& selector_str);

    /**
     * @brief Convert to string format "NAME:NUMBER:ICODE:CHAIN".
     * @return String representation.
     */
    [[nodiscard]] std::string ToString() const;

    /// @brief Comparison by (chain, residue_number, insert_code).
    bool operator<(const Selector& other) const;
    bool operator>(const Selector& other) const;
    bool operator<=(const Selector& other) const;
    bool operator>=(const Selector& other) const;
    bool operator==(const Selector& other) const;
    bool operator!=(const Selector& other) const;
};

/**
 * @brief Predicate that matches atoms belonging to specific residues.
 *
 * Accepts a selector string (comma/semicolon/newline-separated) or a set
 * of Selector objects. Compatible with OpenEye's predicate interface.
 *
 * @code
 * OEResidueSelector sel("ALA:123: :A,GLY:124: :A");
 * for (auto atom = mol.GetAtoms(sel); atom; ++atom) {
 *     // Process matching atoms
 * }
 * @endcode
 */
class OEResidueSelector : public OESystem::OEUnaryPredicate<OEChem::OEAtomBase> {
public:
    /**
     * @brief Construct from a selector string.
     * @param selector_str Comma/semicolon/tab/newline-separated selector strings.
     */
    explicit OEResidueSelector(const std::string& selector_str);

    /**
     * @brief Construct from a set of Selector objects.
     * @param selectors Set of selectors to match against.
     */
    explicit OEResidueSelector(const std::set<Selector>& selectors);

    /// @brief Copy constructor.
    OEResidueSelector(const OEResidueSelector& other);

    /// @brief Destructor.
    ~OEResidueSelector() override;

    /**
     * @brief Test if an atom belongs to one of the specified residues.
     * @param atom The atom to test.
     * @return true if the atom's residue matches any selector.
     */
    bool operator()(const OEChem::OEAtomBase& atom) const override;

    /**
     * @brief Create a copy for OpenEye compatibility.
     * @return New instance (caller takes ownership).
     */
    [[nodiscard]] OESystem::OEUnaryFunction<OEChem::OEAtomBase, bool>*
    CreateCopy() const override;

private:
    std::set<Selector> selectors_;
};

// ============================================================================
// Utility functions
// ============================================================================

/**
 * @brief Parse a selector string into a set of Selector objects.
 *
 * Splits on comma, semicolon, ampersand, tab, and newline delimiters.
 *
 * @param selector_str String containing one or more selectors.
 * @return Set of parsed Selector objects.
 * @throws SelectionError if any selector has invalid format.
 */
std::set<Selector> ParseSelectorSet(const std::string& selector_str);

/**
 * @brief Extract unique Selector objects from a molecule.
 *
 * Iterates all atoms (or atoms matching an optional predicate) and
 * collects their unique residue selectors.
 *
 * @param mol The molecule to extract selectors from.
 * @return Set of unique Selector objects.
 */
std::set<Selector> MolToSelectorSet(const OEChem::OEMolBase& mol);

/**
 * @brief Extract unique selector strings for atoms matching a selection.
 *
 * Applies the selection to the molecule and collects unique selector
 * strings in "NAME:NUMBER:ICODE:CHAIN" format.
 *
 * @param mol The molecule to evaluate.
 * @param selection_str PyMOL-style selection string.
 * @return Set of unique selector strings.
 */
std::set<std::string> StrSelectorSet(
    OEChem::OEMolBase& mol, const std::string& selection_str);

/**
 * @brief Get the selector string for a single atom.
 *
 * @param atom The atom to extract selector from.
 * @return Selector string in "NAME:NUMBER:ICODE:CHAIN" format.
 */
std::string GetSelectorString(const OEChem::OEAtomBase& atom);

}  // namespace OESel

#endif  // OESELECT_RESIDUESELECTOR_H
