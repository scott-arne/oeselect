// src/Selection.cpp
#include "oeselect/Selection.h"
#include "oeselect/Parser.h"
#include "oeselect/Error.h"

namespace OESel {

// TruePredicate as default (matches all atoms)
class TruePredicate : public Predicate {
public:
    bool Evaluate(Context&, const OEChem::OEAtomBase&) const override {
        return true;
    }
    std::string ToCanonical() const override { return "all"; }
    PredicateType Type() const override { return PredicateType::True; }
};

struct OESelection::Impl {
    Predicate::Ptr root;

    Impl() : root(std::make_shared<TruePredicate>()) {}
    explicit Impl(Predicate::Ptr r) : root(std::move(r)) {}
};

OESelection OESelection::Parse(const std::string& sele) {
    if (sele.empty()) {
        return OESelection();
    }
    auto root = ParseSelection(sele);
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
// Helper function to recursively check for predicate type
bool containsPredicateRecursive(const Predicate& pred, PredicateType type) {
    if (pred.Type() == type) {
        return true;
    }
    for (const auto& child : pred.Children()) {
        if (containsPredicateRecursive(*child, type)) {
            return true;
        }
    }
    return false;
}
}  // namespace

bool OESelection::ContainsPredicate(PredicateType type) const {
    return containsPredicateRecursive(*pimpl_->root, type);
}

const Predicate& OESelection::Root() const {
    return *pimpl_->root;
}

bool OESelection::IsEmpty() const {
    return pimpl_->root->Type() == PredicateType::True;
}

}  // namespace OESel
