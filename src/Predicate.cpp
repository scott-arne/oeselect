// src/Predicate.cpp
#include "oeselect/Predicate.h"
#include "oeselect/predicates/NamePredicate.h"
#include "oeselect/predicates/LogicalPredicates.h"
#include "oeselect/Context.h"

#include <oechem.h>
#include <fnmatch.h>
#include <algorithm>

namespace OESel {

// NamePredicate implementation

NamePredicate::NamePredicate(std::string pattern)
    : pattern_(std::move(pattern))
    , has_wildcard_(pattern_.find('*') != std::string::npos ||
                    pattern_.find('?') != std::string::npos) {}

bool NamePredicate::Evaluate(Context&, const OEChem::OEAtomBase& atom) const {
    std::string name = atom.GetName();

    if (has_wildcard_) {
        return fnmatch(pattern_.c_str(), name.c_str(), 0) == 0;
    }
    return name == pattern_;
}

std::string NamePredicate::ToCanonical() const {
    return "name " + pattern_;
}

// AndPredicate implementation

AndPredicate::AndPredicate(std::vector<Ptr> children)
    : children_(std::move(children)) {}

bool AndPredicate::Evaluate(Context& ctx, const OEChem::OEAtomBase& atom) const {
    for (const auto& child : children_) {
        if (!child->Evaluate(ctx, atom)) {
            return false;  // Short-circuit
        }
    }
    return true;
}

std::string AndPredicate::ToCanonical() const {
    if (children_.empty()) return "all";
    if (children_.size() == 1) return children_[0]->ToCanonical();

    std::vector<std::string> parts;
    for (const auto& child : children_) {
        parts.push_back(child->ToCanonical());
    }
    std::sort(parts.begin(), parts.end());

    std::string result = "(" + parts[0];
    for (size_t i = 1; i < parts.size(); ++i) {
        result += " and " + parts[i];
    }
    return result + ")";
}

// OrPredicate implementation

OrPredicate::OrPredicate(std::vector<Ptr> children)
    : children_(std::move(children)) {}

bool OrPredicate::Evaluate(Context& ctx, const OEChem::OEAtomBase& atom) const {
    for (const auto& child : children_) {
        if (child->Evaluate(ctx, atom)) {
            return true;  // Short-circuit
        }
    }
    return false;
}

std::string OrPredicate::ToCanonical() const {
    if (children_.empty()) return "none";
    if (children_.size() == 1) return children_[0]->ToCanonical();

    std::vector<std::string> parts;
    for (const auto& child : children_) {
        parts.push_back(child->ToCanonical());
    }
    std::sort(parts.begin(), parts.end());

    std::string result = "(" + parts[0];
    for (size_t i = 1; i < parts.size(); ++i) {
        result += " or " + parts[i];
    }
    return result + ")";
}

// NotPredicate implementation

NotPredicate::NotPredicate(Ptr child) : child_(std::move(child)) {}

bool NotPredicate::Evaluate(Context& ctx, const OEChem::OEAtomBase& atom) const {
    return !child_->Evaluate(ctx, atom);
}

std::string NotPredicate::ToCanonical() const {
    return "not " + child_->ToCanonical();
}

// XOrPredicate implementation

XOrPredicate::XOrPredicate(std::vector<Ptr> children)
    : children_(std::move(children)) {}

bool XOrPredicate::Evaluate(Context& ctx, const OEChem::OEAtomBase& atom) const {
    int match_count = 0;
    for (const auto& child : children_) {
        if (child->Evaluate(ctx, atom)) {
            match_count++;
            if (match_count > 1) return false;  // Early exit
        }
    }
    return match_count == 1;
}

std::string XOrPredicate::ToCanonical() const {
    if (children_.size() < 2) {
        return children_.empty() ? "none" : children_[0]->ToCanonical();
    }

    std::vector<std::string> parts;
    for (const auto& child : children_) {
        parts.push_back(child->ToCanonical());
    }
    std::sort(parts.begin(), parts.end());

    std::string result = "(" + parts[0];
    for (size_t i = 1; i < parts.size(); ++i) {
        result += " xor " + parts[i];
    }
    return result + ")";
}

}  // namespace OESel
