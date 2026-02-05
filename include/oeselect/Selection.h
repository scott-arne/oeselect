/**
 * @file Selection.h
 * @brief Immutable, parsed selection representation.
 *
 * OESelection represents a parsed selection expression that can be applied
 * to multiple molecules. The selection is immutable and thread-safe once
 * constructed.
 */

#ifndef OESELECT_SELECTION_H
#define OESELECT_SELECTION_H

#include <memory>
#include <string>

#include "oeselect/Predicate.h"

namespace OESel {

/**
 * @brief Immutable, thread-safe parsed selection.
 *
 * OESelection holds a parsed predicate tree that can be used with OESelect
 * to filter atoms. Once created, the selection is immutable and can be
 * safely shared across threads.
 *
 * @code
 * // Parse a selection string
 * auto sele = OESelection::Parse("protein and chain A");
 *
 * // Check what predicates are used
 * if (sele.ContainsPredicate(PredicateType::Around)) {
 *     // Spatial index will be needed
 * }
 *
 * // Get canonical form for comparison
 * std::cout << sele.ToCanonical() << std::endl;
 * @endcode
 */
class OESelection {
public:
    /**
     * @brief Parse a selection string into an OESelection.
     *
     * @param sele PyMOL-style selection string.
     * @return Parsed selection object.
     * @throws SelectionError if parsing fails.
     *
     * @see Parser.h for supported selection syntax.
     */
    static OESelection Parse(const std::string& sele);

    /**
     * @brief Default constructor creates an empty selection.
     *
     * An empty selection matches all atoms (equivalent to "all" keyword).
     */
    OESelection();

    /// @brief Copy constructor.
    OESelection(const OESelection& other);

    /// @brief Move constructor.
    OESelection(OESelection&& other) noexcept;

    /// @brief Copy assignment operator.
    OESelection& operator=(const OESelection& other);

    /// @brief Move assignment operator.
    OESelection& operator=(OESelection&& other) noexcept;

    /// @brief Destructor.
    ~OESelection();

    /**
     * @brief Get canonical string representation.
     *
     * Returns a normalized form of the selection suitable for comparison
     * and display. AND/OR children are sorted alphabetically.
     *
     * @return Canonical selection string.
     */
    std::string ToCanonical() const;

    /**
     * @brief Check if selection contains a predicate of given type.
     *
     * Useful for determining what resources may be needed for evaluation.
     * For example, distance predicates require a spatial index.
     *
     * @param type The predicate type to search for.
     * @return true if the selection tree contains this predicate type.
     */
    bool ContainsPredicate(PredicateType type) const;

    /**
     * @brief Access the root predicate for direct evaluation.
     *
     * @return Reference to the root predicate.
     * @note Prefer using OESelect for atom evaluation.
     */
    const Predicate& Root() const;

    /**
     * @brief Check if this is an empty selection.
     *
     * An empty selection (created by default constructor or Parse(""))
     * matches all atoms.
     *
     * @return true if this selection matches all atoms.
     */
    bool IsEmpty() const;

private:
    /// @brief Private constructor from root predicate.
    explicit OESelection(Predicate::Ptr root);

    struct Impl;
    std::unique_ptr<Impl> pimpl_;  ///< PIMPL for binary compatibility
};

}  // namespace OESel

#endif  // OESELECT_SELECTION_H
