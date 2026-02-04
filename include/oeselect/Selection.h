// include/oeselect/Selection.h
#ifndef OESELECT_SELECTION_H
#define OESELECT_SELECTION_H

#include <memory>
#include <string>

#include "oeselect/Predicate.h"

namespace OESel {

/// Immutable, thread-safe parsed selection
class OESelection {
public:
    /// Parse a selection string (throws SelectionError on failure)
    static OESelection Parse(const std::string& sele);

    /// Default constructor (creates empty/all selection)
    OESelection();

    /// Copy/move constructors
    OESelection(const OESelection& other);
    OESelection(OESelection&& other) noexcept;
    OESelection& operator=(const OESelection& other);
    OESelection& operator=(OESelection&& other) noexcept;
    ~OESelection();

    /// Get canonical string representation
    std::string ToCanonical() const;

    /// Check if selection contains a predicate of given type
    bool ContainsPredicate(PredicateType type) const;

    /// Access root predicate (for evaluation)
    const Predicate& Root() const;

    /// Check if selection is empty (matches all atoms)
    bool IsEmpty() const;

private:
    explicit OESelection(Predicate::Ptr root);

    struct Impl;
    std::unique_ptr<Impl> pimpl_;
};

}  // namespace OESel

#endif  // OESELECT_SELECTION_H
