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
    And,    ///< Logical AND of child predicates
    Or,     ///< Logical OR of child predicates
    Not,    ///< Logical negation of child predicate
    XOr,    ///< Exclusive OR of child predicates

    // Atom property predicates
    Name,   ///< Atom name matching (supports wildcards)
    Resn,   ///< Residue name matching
    Resi,   ///< Residue number (supports ranges/comparisons)
    Chain,  ///< Chain identifier matching
    Elem,   ///< Element symbol matching
    Index,  ///< Atom index (supports ranges/comparisons)
    SecondaryStructure,  ///< Secondary structure type

    // Molecular component predicates
    Protein,    ///< Protein atoms
    Ligand,     ///< Small molecule ligand atoms
    Water,      ///< Water molecules
    Solvent,    ///< Solvent molecules (water + common solvents)
    Organic,    ///< Organic small molecules
    Backbone,   ///< Protein backbone atoms (N, CA, C, O)
    Metal,      ///< Metal ions

    // Atom type predicates
    Heavy,              ///< Non-hydrogen atoms
    Hydrogen,           ///< All hydrogen atoms
    PolarHydrogen,      ///< Hydrogens bonded to N, O, S
    NonpolarHydrogen,   ///< Hydrogens bonded to C

    // Expansion operators
    ByRes,      ///< Expand selection to complete residues
    ByChain,    ///< Expand selection to complete chains

    // Distance operators
    Around,     ///< Atoms within distance of selection
    XAround,    ///< Around excluding reference atoms
    Box,        ///< Bounding box distance (faster approximation)
    XBox,       ///< Box excluding reference atoms
    Beyond,     ///< Atoms outside distance of selection

    // Secondary structure types
    Helix,  ///< Alpha helix
    Sheet,  ///< Beta sheet
    Turn,   ///< Turn
    Loop,   ///< Loop/coil

    // Constants
    True,   ///< Always matches (used for empty/all selections)
    False   ///< Never matches (used for 'none' keyword)
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
    virtual std::string ToCanonical() const = 0;

    /**
     * @brief Get the type of this predicate for introspection.
     * @return The PredicateType enum value.
     */
    virtual PredicateType Type() const = 0;

    /**
     * @brief Get child predicates for composite predicates.
     *
     * For leaf predicates (Name, Elem, etc.), returns empty vector.
     * For composite predicates (And, Or, Not), returns children.
     *
     * @return Vector of child predicate pointers.
     */
    virtual std::vector<Ptr> Children() const { return {}; }
};

}  // namespace OESel

#endif  // OESELECT_PREDICATE_H
