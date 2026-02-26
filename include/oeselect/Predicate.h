/**
 * @file Predicate.h
 * @brief Base predicate class and predicate type enumeration.
 *
 * Predicates form the building blocks of selections. Each predicate
 * evaluates to true or false for a given atom in context.
 */

#ifndef OESELECT_PREDICATE_H
#define OESELECT_PREDICATE_H

#include <memory>
#include <string>
#include <vector>

namespace OEChem {
class OEAtomBase;
}

namespace OESel {

class Context;

/**
 * @brief Enumeration of all predicate types for introspection.
 *
 * Use with OESelection::ContainsPredicate() to check if a selection
 * uses specific features (e.g., distance operators, component types).
 */
enum class PredicateType {
    // Logical operators
    AND,    ///< Logical AND of child predicates
    OR,     ///< Logical OR of child predicates
    NOT,    ///< Logical negation of child predicate
    XOR,    ///< Exclusive OR of child predicates

    // Atom property predicates
    NAME,   ///< Atom name matching (supports wildcards)
    RESN,   ///< Residue name matching
    RESI,   ///< Residue number (supports ranges/comparisons)
    CHAIN,  ///< Chain identifier matching
    ELEM,   ///< Element symbol matching
    INDEX,  ///< Atom index (supports ranges/comparisons)
    ID,     ///< Atom serial number (supports ranges/comparisons)
    ALT,    ///< Alternate location identifier
    B_FACTOR,       ///< B-factor / temperature factor (supports float ranges/comparisons)
    FRAGMENT,       ///< Fragment number (supports ranges/comparisons)
    SECONDARY_STRUCTURE,  ///< Secondary structure type

    // Molecular component predicates
    PROTEIN,    ///< Protein atoms
    LIGAND,     ///< Small molecule ligand atoms
    WATER,      ///< Water molecules
    SOLVENT,    ///< Solvent molecules (water + common solvents)
    ORGANIC,    ///< Organic small molecules
    BACKBONE,   ///< Protein backbone atoms (N, CA, C, O)
    METAL,      ///< Metal ions

    // Atom type predicates
    HEAVY,              ///< Non-hydrogen atoms
    HYDROGEN,           ///< All hydrogen atoms
    POLAR_HYDROGEN,     ///< Hydrogens bonded to N, O, S
    NONPOLAR_HYDROGEN,  ///< Hydrogens bonded to C

    // Expansion operators
    BY_RES,     ///< Expand selection to complete residues
    BY_CHAIN,   ///< Expand selection to complete chains

    // Distance operators
    AROUND,     ///< Atoms within distance of selection, excluding reference
    EXPAND,     ///< Atoms within distance of selection, including reference
    BEYOND,     ///< Atoms outside distance of selection

    // Secondary structure types
    HELIX,  ///< Alpha helix
    SHEET,  ///< Beta sheet
    TURN,   ///< Turn
    LOOP,   ///< Loop/coil

    // Constants
    TRUE,   ///< Always matches (used for empty/all selections)
    FALSE   ///< Never matches (used for 'none' keyword)
};

/**
 * @brief Abstract base class for all selection predicates.
 *
 * Predicates are immutable once constructed and form a tree structure
 * representing parsed selection expressions. Composite predicates
 * (And, Or, Not) contain child predicates.
 *
 * @note All predicate implementations must be thread-safe for Evaluate().
 */
class Predicate {
public:
    /// @brief Shared pointer type for predicate ownership
    using Ptr = std::shared_ptr<Predicate>;

    virtual ~Predicate() = default;

    /**
     * @brief Evaluate this predicate for a specific atom.
     *
     * @param ctx Evaluation context containing molecule and caches.
     * @param atom The atom to evaluate.
     * @return true if the atom matches the predicate.
     *
     * @note Implementations may use ctx for caching results.
     */
    virtual bool Evaluate(Context& ctx, const OEChem::OEAtomBase& atom) const = 0;

    /**
     * @brief Get canonical string representation of this predicate.
     *
     * Returns a normalized form suitable for comparison and display.
     * Child predicates in AND/OR are sorted alphabetically.
     *
     * @return Canonical selection string.
     */
    [[nodiscard]] virtual std::string ToCanonical() const = 0;

    /**
     * @brief Get the type of this predicate for introspection.
     * @return The PredicateType enum value.
     */
    [[nodiscard]] virtual PredicateType Type() const = 0;

    /**
     * @brief Get child predicates for composite predicates.
     *
     * For leaf predicates (Name, Elem, etc.), returns empty vector.
     * For composite predicates (And, Or, Not), returns children.
     *
     * @return Vector of child predicate pointers.
     */
    [[nodiscard]] virtual std::vector<Ptr> Children() const { return {}; }
};

}  // namespace OESel

#endif  // OESELECT_PREDICATE_H
