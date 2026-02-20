/**
 * @file Selection.cpp
 * @brief OESelection implementation.
 */

#include "oeselect/Selection.h"
#include "oeselect/Parser.h"

#include <algorithm>

namespace OESel {

/**
 * @brief Internal predicate that always returns true.
 *
 * Used as the default root predicate for empty selections,
 * effectively selecting all atoms.
 */
class TruePredicate : public Predicate {
public:
    bool Evaluate(Context&, const OEChem::OEAtomBase&) const override {
        return true;
    }
    [[nodiscard]] std::string ToCanonical() const override { return "all"; }
    [[nodiscard]] PredicateType Type() const override { return PredicateType::True; }
};

/// PIMPL implementation holding the root predicate
struct OESelection::Impl {
    Predicate::Ptr root;

    Impl() : root(std::make_shared<TruePredicate>()) {}
    explicit Impl(Predicate::Ptr r) : root(std::move(r)) {}
};

OESelection OESelection::Parse(const std::string& sele) {
    if (sele.empty()) {
        return {};  // Empty string = select all
    }
    const auto root = ParseSelection(sele);
    return OESelection(root);
}

OESelection::OESelection() : pimpl_(std::make_unique<Impl>()) {}

OESelection::OESelection(Predicate::Ptr root)
    : pimpl_(std::make_unique<Impl>(std::move(root))) {}

OESelection::OESelection(const OESelection& other)
    : pimpl_(std::make_unique<Impl>(*other.pimpl_)) {}

OESelection::OESelection(OESelection&& other) noexcept = default;

OESelection& OESelection::operator=(const OESelection& other) {
    if (this != &other) {
        pimpl_ = std::make_unique<Impl>(*other.pimpl_);
    }
    return *this;
}

OESelection& OESelection::operator=(OESelection&& other) noexcept = default;

OESelection::~OESelection() = default;

std::string OESelection::ToCanonical() const {
    return pimpl_->root->ToCanonical();
}

namespace {
/**
 * @brief Recursively search predicate tree for a specific type.
 *
 * @param pred Root of subtree to search.
 * @param type Target predicate type.
 * @return true if any predicate in the subtree has the target type.
 */
bool containsPredicateRecursive(const Predicate& pred, const PredicateType type) {
    if (pred.Type() == type) {
        return true;
    }
    const auto& children = pred.Children();
    return std::any_of(children.begin(), children.end(),
        [type](const auto& child) { return containsPredicateRecursive(*child, type); });
}
}  // namespace

bool OESelection::ContainsPredicate(const PredicateType type) const {
    return containsPredicateRecursive(*pimpl_->root, type);
}

const Predicate& OESelection::Root() const {
    return *pimpl_->root;
}

bool OESelection::IsEmpty() const {
    return pimpl_->root->Type() == PredicateType::True;
}

}  // namespace OESel
