// src/Predicate.cpp
#include "oeselect/Predicate.h"
#include "oeselect/predicates/NamePredicate.h"
#include "oeselect/predicates/LogicalPredicates.h"
#include "oeselect/predicates/AtomPropertyPredicates.h"
#include "oeselect/predicates/ComponentPredicates.h"
#include "oeselect/predicates/AtomTypePredicates.h"
#include "oeselect/Context.h"
#include "oeselect/Tagger.h"

#include <oechem.h>
#include <fnmatch.h>
#include <algorithm>
#include <cctype>
#include <string>

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

// ResnPredicate implementation

ResnPredicate::ResnPredicate(std::string pattern)
    : pattern_(std::move(pattern))
    , has_wildcard_(pattern_.find('*') != std::string::npos ||
                    pattern_.find('?') != std::string::npos) {}

bool ResnPredicate::Evaluate(Context&, const OEChem::OEAtomBase& atom) const {
    const OEChem::OEResidue& res = OEChem::OEAtomGetResidue(&atom);

    std::string resn = res.GetName();
    if (has_wildcard_) {
        return fnmatch(pattern_.c_str(), resn.c_str(), 0) == 0;
    }
    return resn == pattern_;
}

std::string ResnPredicate::ToCanonical() const {
    return "resn " + pattern_;
}

// ResiPredicate implementation

ResiPredicate::ResiPredicate(int value, Op op)
    : value_(value), end_value_(0), op_(op) {}

ResiPredicate::ResiPredicate(int start, int end)
    : value_(start), end_value_(end), op_(Op::Range) {}

bool ResiPredicate::Evaluate(Context&, const OEChem::OEAtomBase& atom) const {
    const OEChem::OEResidue& res = OEChem::OEAtomGetResidue(&atom);

    int resi = res.GetResidueNumber();
    switch (op_) {
        case Op::Eq:    return resi == value_;
        case Op::Lt:    return resi < value_;
        case Op::Le:    return resi <= value_;
        case Op::Gt:    return resi > value_;
        case Op::Ge:    return resi >= value_;
        case Op::Range: return resi >= value_ && resi <= end_value_;
    }
    return false;  // Should never reach here
}

std::string ResiPredicate::ToCanonical() const {
    switch (op_) {
        case Op::Eq:    return "resi " + std::to_string(value_);
        case Op::Lt:    return "resi < " + std::to_string(value_);
        case Op::Le:    return "resi <= " + std::to_string(value_);
        case Op::Gt:    return "resi > " + std::to_string(value_);
        case Op::Ge:    return "resi >= " + std::to_string(value_);
        case Op::Range: return "resi " + std::to_string(value_) + "-" + std::to_string(end_value_);
    }
    return "resi " + std::to_string(value_);  // Default fallback
}

// ChainPredicate implementation

ChainPredicate::ChainPredicate(std::string chain_id)
    : chain_id_(std::move(chain_id)) {}

bool ChainPredicate::Evaluate(Context&, const OEChem::OEAtomBase& atom) const {
    const OEChem::OEResidue& res = OEChem::OEAtomGetResidue(&atom);

    char chain = res.GetChainID();
    return chain_id_.size() == 1 && chain == chain_id_[0];
}

std::string ChainPredicate::ToCanonical() const {
    return "chain " + chain_id_;
}

// ElemPredicate implementation

namespace {
// Helper to convert element symbol to uppercase for canonical form
std::string NormalizeElement(const std::string& elem) {
    if (elem.empty()) return elem;
    std::string result;
    result += static_cast<char>(std::toupper(static_cast<unsigned char>(elem[0])));
    for (size_t i = 1; i < elem.size(); ++i) {
        result += static_cast<char>(std::tolower(static_cast<unsigned char>(elem[i])));
    }
    return result;
}
}  // namespace

ElemPredicate::ElemPredicate(std::string element)
    : atomic_num_(OEChem::OEGetAtomicNum(element.c_str()))
    , element_(NormalizeElement(element)) {}

bool ElemPredicate::Evaluate(Context&, const OEChem::OEAtomBase& atom) const {
    return atom.GetAtomicNum() == static_cast<int>(atomic_num_);
}

std::string ElemPredicate::ToCanonical() const {
    return "elem " + element_;
}

// IndexPredicate implementation

IndexPredicate::IndexPredicate(unsigned int value)
    : value_(value), end_value_(0), is_range_(false) {}

IndexPredicate::IndexPredicate(unsigned int start, unsigned int end)
    : value_(start), end_value_(end), is_range_(true) {}

bool IndexPredicate::Evaluate(Context&, const OEChem::OEAtomBase& atom) const {
    unsigned int idx = atom.GetIdx();
    if (is_range_) {
        return idx >= value_ && idx <= end_value_;
    }
    return idx == value_;
}

std::string IndexPredicate::ToCanonical() const {
    if (is_range_) {
        return "index " + std::to_string(value_) + "-" + std::to_string(end_value_);
    }
    return "index " + std::to_string(value_);
}

// ProteinPredicate implementation

bool ProteinPredicate::Evaluate(Context& ctx, const OEChem::OEAtomBase& atom) const {
    Tagger::TagMolecule(ctx.Mol());
    return Tagger::HasComponent(atom, ComponentFlag::Protein);
}

// LigandPredicate implementation

bool LigandPredicate::Evaluate(Context& ctx, const OEChem::OEAtomBase& atom) const {
    Tagger::TagMolecule(ctx.Mol());
    return Tagger::HasComponent(atom, ComponentFlag::Ligand);
}

// WaterPredicate implementation

bool WaterPredicate::Evaluate(Context& ctx, const OEChem::OEAtomBase& atom) const {
    Tagger::TagMolecule(ctx.Mol());
    return Tagger::HasComponent(atom, ComponentFlag::Water);
}

// SolventPredicate implementation - matches Water OR Solvent

bool SolventPredicate::Evaluate(Context& ctx, const OEChem::OEAtomBase& atom) const {
    Tagger::TagMolecule(ctx.Mol());
    return Tagger::HasComponent(atom, ComponentFlag::Water) ||
           Tagger::HasComponent(atom, ComponentFlag::Solvent);
}

// OrganicPredicate implementation - C-containing, not protein/nucleic

bool OrganicPredicate::Evaluate(Context& ctx, const OEChem::OEAtomBase& atom) const {
    // Check if atom is carbon or bonded to carbon (i.e., in organic molecule)
    if (atom.GetAtomicNum() != 6) {  // Not carbon itself
        // Check if bonded to carbon
        bool bonded_to_carbon = false;
        for (OESystem::OEIter<OEChem::OEBondBase> bond = atom.GetBonds(); bond; ++bond) {
            const OEChem::OEAtomBase* nbr = bond->GetNbr(&atom);
            if (nbr->GetAtomicNum() == 6) {
                bonded_to_carbon = true;
                break;
            }
        }
        if (!bonded_to_carbon) return false;
    }

    Tagger::TagMolecule(ctx.Mol());
    // Exclude protein and nucleic
    if (Tagger::HasComponent(atom, ComponentFlag::Protein)) return false;
    if (Tagger::HasComponent(atom, ComponentFlag::Nucleic)) return false;
    return true;
}

// BackbonePredicate implementation - N, CA, C, O in protein

bool BackbonePredicate::Evaluate(Context& ctx, const OEChem::OEAtomBase& atom) const {
    Tagger::TagMolecule(ctx.Mol());
    if (!Tagger::HasComponent(atom, ComponentFlag::Protein)) return false;

    std::string name = atom.GetName();
    return name == "N" || name == "CA" || name == "C" || name == "O";
}

// SidechainPredicate implementation - protein atoms that are not backbone

bool SidechainPredicate::Evaluate(Context& ctx, const OEChem::OEAtomBase& atom) const {
    Tagger::TagMolecule(ctx.Mol());
    if (!Tagger::HasComponent(atom, ComponentFlag::Protein)) return false;

    std::string name = atom.GetName();
    // Exclude backbone atoms
    if (name == "N" || name == "CA" || name == "C" || name == "O") return false;
    // Also exclude OXT (terminal oxygen)
    if (name == "OXT") return false;
    return true;
}

// MetalPredicate implementation - metal elements

bool MetalPredicate::Evaluate(Context&, const OEChem::OEAtomBase& atom) const {
    int atomicNum = atom.GetAtomicNum();
    // Common metals in biomolecular structures
    // Li(3), Na(11), Mg(12), Al(13), K(19), Ca(20), Sc(21)...Zn(30),
    // Rb(37)...Cd(48), Cs(55)...Hg(80)
    return (atomicNum == 3) ||   // Li
           (atomicNum >= 11 && atomicNum <= 13) ||  // Na, Mg, Al
           (atomicNum >= 19 && atomicNum <= 30) ||  // K through Zn
           (atomicNum >= 37 && atomicNum <= 48) ||  // Rb through Cd
           (atomicNum >= 55 && atomicNum <= 80);    // Cs through Hg
}

// HeavyPredicate implementation - non-hydrogen atoms

bool HeavyPredicate::Evaluate(Context&, const OEChem::OEAtomBase& atom) const {
    return atom.GetAtomicNum() > 1;
}

// HydrogenPredicate implementation - hydrogen atoms

bool HydrogenPredicate::Evaluate(Context&, const OEChem::OEAtomBase& atom) const {
    return atom.GetAtomicNum() == 1;
}

// PolarHydrogenPredicate implementation - H bonded to N(7), O(8), or S(16)

bool PolarHydrogenPredicate::Evaluate(Context&, const OEChem::OEAtomBase& atom) const {
    if (atom.GetAtomicNum() != 1) return false;  // Must be hydrogen

    // Check what this hydrogen is bonded to
    for (OESystem::OEIter<OEChem::OEBondBase> bond = atom.GetBonds(); bond; ++bond) {
        const OEChem::OEAtomBase* nbr = bond->GetNbr(&atom);
        int nbrAtomicNum = nbr->GetAtomicNum();
        if (nbrAtomicNum == 7 || nbrAtomicNum == 8 || nbrAtomicNum == 16) {
            return true;  // Bonded to N, O, or S
        }
    }
    return false;
}

// NonpolarHydrogenPredicate implementation - H bonded to C (or not N/O/S)

bool NonpolarHydrogenPredicate::Evaluate(Context&, const OEChem::OEAtomBase& atom) const {
    if (atom.GetAtomicNum() != 1) return false;  // Must be hydrogen

    // Check what this hydrogen is bonded to
    for (OESystem::OEIter<OEChem::OEBondBase> bond = atom.GetBonds(); bond; ++bond) {
        const OEChem::OEAtomBase* nbr = bond->GetNbr(&atom);
        int nbrAtomicNum = nbr->GetAtomicNum();
        // If bonded to N, O, or S, it's polar
        if (nbrAtomicNum == 7 || nbrAtomicNum == 8 || nbrAtomicNum == 16) {
            return false;
        }
    }
    return true;  // Not bonded to N/O/S, so it's nonpolar
}

}  // namespace OESel
