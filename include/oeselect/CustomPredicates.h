/**
 * @file CustomPredicates.h
 * @brief Custom atom predicates with advanced matching options.
 *
 * Provides predicates for matching residue names and atom names with
 * control over case sensitivity and whitespace handling.
 */

#ifndef OESELECT_CUSTOMPREDICATES_H
#define OESELECT_CUSTOMPREDICATES_H

#include <string>

#include <oechem.h>

namespace OESel {

/**
 * @brief Match atoms by residue name with optional case/whitespace control.
 *
 * Unlike OpenEye's built-in OEHasResidueProperty, this predicate provides
 * control over case sensitivity and whitespace handling in comparisons.
 * By default, comparisons are case-insensitive and whitespace is stripped.
 *
 * @code
 * OEHasResidueName pred("ala");  // Matches "ALA", " ALA", "Ala", etc.
 * OEHasResidueName pred_strict("ALA", true, true);  // Exact match only
 * @endcode
 */
class OEHasResidueName : public OESystem::OEUnaryPredicate<OEChem::OEAtomBase> {
public:
    /**
     * @brief Construct a residue name predicate.
     * @param residue_name The residue name to match.
     * @param case_sensitive If true, comparison is case-sensitive (default: false).
     * @param whitespace If true, whitespace is preserved in comparison (default: false).
     */
    explicit OEHasResidueName(const std::string& residue_name,
                              bool case_sensitive = false,
                              bool whitespace = false);

    /// @brief Copy constructor.
    OEHasResidueName(const OEHasResidueName& other);

    /// @brief Destructor.
    ~OEHasResidueName() override;

    /**
     * @brief Test if an atom's residue name matches.
     * @param atom The atom to test.
     * @return true if the residue name matches.
     */
    bool operator()(const OEChem::OEAtomBase& atom) const override;

    /**
     * @brief Create a copy for OpenEye compatibility.
     * @return New instance (caller takes ownership).
     */
    [[nodiscard]] OESystem::OEUnaryFunction<OEChem::OEAtomBase, bool>*
    CreateCopy() const override;

private:
    std::string residue_name_;
    bool case_sensitive_;
    bool whitespace_;
};

/**
 * @brief Match atoms by name with optional case/whitespace control.
 *
 * More advanced version of OpenEye's OEHasAtomName that provides control
 * over case sensitivity and whitespace handling. By default, comparisons
 * are case-insensitive and whitespace is stripped.
 *
 * @code
 * OEHasAtomNameAdvanced pred("ca");  // Matches "CA", " CA", "Ca", etc.
 * OEHasAtomNameAdvanced pred_strict("CA", true, true);  // Exact match only
 * @endcode
 */
class OEHasAtomNameAdvanced : public OESystem::OEUnaryPredicate<OEChem::OEAtomBase> {
public:
    /**
     * @brief Construct an atom name predicate.
     * @param atom_name The atom name to match.
     * @param case_sensitive If true, comparison is case-sensitive (default: false).
     * @param whitespace If true, whitespace is preserved in comparison (default: false).
     */
    explicit OEHasAtomNameAdvanced(const std::string& atom_name,
                                   bool case_sensitive = false,
                                   bool whitespace = false);

    /// @brief Copy constructor.
    OEHasAtomNameAdvanced(const OEHasAtomNameAdvanced& other);

    /// @brief Destructor.
    ~OEHasAtomNameAdvanced() override;

    /**
     * @brief Test if an atom's name matches.
     * @param atom The atom to test.
     * @return true if the atom name matches.
     */
    bool operator()(const OEChem::OEAtomBase& atom) const override;

    /**
     * @brief Create a copy for OpenEye compatibility.
     * @return New instance (caller takes ownership).
     */
    [[nodiscard]] OESystem::OEUnaryFunction<OEChem::OEAtomBase, bool>*
    CreateCopy() const override;

private:
    std::string atom_name_;
    bool case_sensitive_;
    bool whitespace_;
};

}  // namespace OESel

#endif  // OESELECT_CUSTOMPREDICATES_H
