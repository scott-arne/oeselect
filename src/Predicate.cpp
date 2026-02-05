/**
 * @file Predicate.cpp
 * @brief Predicate implementations for all selection types.
 *
 * This file contains the Evaluate() and ToCanonical() implementations
 * for all predicate classes. Predicates are organized by category:
 * - Name and property predicates
 * - Logical predicates (AND, OR, NOT, XOR)
 * - Component predicates (protein, ligand, etc.)
 * - Atom type predicates (heavy, hydrogen)
 * - Distance predicates (around, xaround, beyond)
 * - Expansion predicates (byres, bychain)
 * - Secondary structure predicates
 */

#include "oeselect/Predicate.h"
#include "oeselect/predicates/NamePredicate.h"
#include "oeselect/predicates/LogicalPredicates.h"
#include "oeselect/predicates/AtomPropertyPredicates.h"
#include "oeselect/predicates/ComponentPredicates.h"
#include "oeselect/predicates/AtomTypePredicates.h"
#include "oeselect/predicates/DistancePredicates.h"
#include "oeselect/predicates/ExpansionPredicates.h"
#include "oeselect/predicates/SecondaryStructurePredicates.h"
#include "oeselect/Context.h"
#include "oeselect/Tagger.h"
#include "oeselect/SpatialIndex.h"

#include <oechem.h>
#include <fnmatch.h>
#include <algorithm>
#include <cctype>
#include <iomanip>
#include <sstream>
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

IndexPredicate::IndexPredicate(unsigned int value, Op op)
    : value_(value), end_value_(0), op_(op) {}

IndexPredicate::IndexPredicate(unsigned int start, unsigned int end)
    : value_(start), end_value_(end), op_(Op::Range) {}

bool IndexPredicate::Evaluate(Context&, const OEChem::OEAtomBase& atom) const {
    unsigned int idx = atom.GetIdx();
    switch (op_) {
        case Op::Eq:    return idx == value_;
        case Op::Lt:    return idx < value_;
        case Op::Le:    return idx <= value_;
        case Op::Gt:    return idx > value_;
        case Op::Ge:    return idx >= value_;
        case Op::Range: return idx >= value_ && idx <= end_value_;
    }
    return false;
}

std::string IndexPredicate::ToCanonical() const {
    switch (op_) {
        case Op::Eq:    return "index " + std::to_string(value_);
        case Op::Lt:    return "index < " + std::to_string(value_);
        case Op::Le:    return "index <= " + std::to_string(value_);
        case Op::Gt:    return "index > " + std::to_string(value_);
        case Op::Ge:    return "index >= " + std::to_string(value_);
        case Op::Range: return "index " + std::to_string(value_) + "-" + std::to_string(end_value_);
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

// ============================================================================
// Distance Predicates (Task 13)
// ============================================================================

namespace {
// Helper to format radius for canonical output
std::string FormatRadius(float radius) {
    std::ostringstream oss;
    oss << std::setprecision(6) << std::noshowpoint << radius;
    std::string result = oss.str();
    // Remove trailing zeros after decimal point
    if (result.find('.') != std::string::npos) {
        size_t last = result.find_last_not_of('0');
        if (last != std::string::npos && result[last] == '.') {
            result = result.substr(0, last);  // Remove decimal point if no decimals
        } else {
            result = result.substr(0, last + 1);
        }
    }
    return result;
}
}  // namespace

// AroundPredicate implementation

AroundPredicate::AroundPredicate(float radius, Ptr reference)
    : radius_(radius), reference_(std::move(reference)) {}

const std::vector<bool>& AroundPredicate::GetAroundMask(Context& ctx) const {
    std::string cache_key = "around_" + FormatRadius(radius_) + "_" + reference_->ToCanonical();

    if (ctx.HasAroundCache(cache_key)) {
        return ctx.GetAroundCache(cache_key);
    }

    OEChem::OEMolBase& mol = ctx.Mol();
    size_t num_atoms = mol.NumAtoms();
    std::vector<bool> mask(num_atoms, false);

    // Get spatial index
    SpatialIndex& index = ctx.GetSpatialIndex();

    // Find all reference atoms and collect nearby atoms
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
        if (reference_->Evaluate(ctx, *atom)) {
            // This is a reference atom - find all atoms within radius
            auto nearby = index.FindWithinRadius(*atom, radius_);
            for (unsigned int idx : nearby) {
                if (idx < mask.size()) {
                    mask[idx] = true;
                }
            }
        }
    }

    ctx.SetAroundCache(cache_key, std::move(mask));
    return ctx.GetAroundCache(cache_key);
}

bool AroundPredicate::Evaluate(Context& ctx, const OEChem::OEAtomBase& atom) const {
    const auto& mask = GetAroundMask(ctx);
    unsigned int idx = atom.GetIdx();
    return idx < mask.size() && mask[idx];
}

std::string AroundPredicate::ToCanonical() const {
    return "around " + FormatRadius(radius_) + " " + reference_->ToCanonical();
}

// XAroundPredicate implementation

XAroundPredicate::XAroundPredicate(float radius, Ptr reference)
    : radius_(radius), reference_(std::move(reference)) {}

const std::vector<bool>& XAroundPredicate::GetAroundMask(Context& ctx) const {
    // Use same cache key format as AroundPredicate since it's the same computation
    std::string cache_key = "around_" + FormatRadius(radius_) + "_" + reference_->ToCanonical();

    if (ctx.HasAroundCache(cache_key)) {
        return ctx.GetAroundCache(cache_key);
    }

    OEChem::OEMolBase& mol = ctx.Mol();
    size_t num_atoms = mol.NumAtoms();
    std::vector<bool> mask(num_atoms, false);

    // Get spatial index
    SpatialIndex& index = ctx.GetSpatialIndex();

    // Find all reference atoms and collect nearby atoms
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
        if (reference_->Evaluate(ctx, *atom)) {
            // This is a reference atom - find all atoms within radius
            auto nearby = index.FindWithinRadius(*atom, radius_);
            for (unsigned int idx : nearby) {
                if (idx < mask.size()) {
                    mask[idx] = true;
                }
            }
        }
    }

    ctx.SetAroundCache(cache_key, std::move(mask));
    return ctx.GetAroundCache(cache_key);
}

bool XAroundPredicate::Evaluate(Context& ctx, const OEChem::OEAtomBase& atom) const {
    // First check if this atom is in the reference selection - if so, exclude it
    if (reference_->Evaluate(ctx, atom)) {
        return false;
    }
    // Then check if it's within radius
    const auto& mask = GetAroundMask(ctx);
    unsigned int idx = atom.GetIdx();
    return idx < mask.size() && mask[idx];
}

std::string XAroundPredicate::ToCanonical() const {
    return "xaround " + FormatRadius(radius_) + " " + reference_->ToCanonical();
}

// BeyondPredicate implementation

BeyondPredicate::BeyondPredicate(float radius, Ptr reference)
    : radius_(radius), reference_(std::move(reference)) {}

const std::vector<bool>& BeyondPredicate::GetAroundMask(Context& ctx) const {
    // Use the same cache as around - we just interpret it differently
    std::string cache_key = "around_" + FormatRadius(radius_) + "_" + reference_->ToCanonical();

    if (ctx.HasAroundCache(cache_key)) {
        return ctx.GetAroundCache(cache_key);
    }

    OEChem::OEMolBase& mol = ctx.Mol();
    size_t num_atoms = mol.NumAtoms();
    std::vector<bool> mask(num_atoms, false);

    // Get spatial index
    SpatialIndex& index = ctx.GetSpatialIndex();

    // Find all reference atoms and collect nearby atoms
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
        if (reference_->Evaluate(ctx, *atom)) {
            // This is a reference atom - find all atoms within radius
            auto nearby = index.FindWithinRadius(*atom, radius_);
            for (unsigned int idx : nearby) {
                if (idx < mask.size()) {
                    mask[idx] = true;
                }
            }
        }
    }

    ctx.SetAroundCache(cache_key, std::move(mask));
    return ctx.GetAroundCache(cache_key);
}

bool BeyondPredicate::Evaluate(Context& ctx, const OEChem::OEAtomBase& atom) const {
    // Beyond is the inverse of around - atom is beyond if it's NOT in the around mask
    const auto& mask = GetAroundMask(ctx);
    unsigned int idx = atom.GetIdx();
    // If idx is out of range, consider it beyond (shouldn't happen normally)
    if (idx >= mask.size()) {
        return true;
    }
    return !mask[idx];
}

std::string BeyondPredicate::ToCanonical() const {
    return "beyond " + FormatRadius(radius_) + " " + reference_->ToCanonical();
}

// ============================================================================
// Expansion Predicates (Task 14)
// ============================================================================

// ByResPredicate implementation

namespace {
// Helper struct to create a unique residue key
// A residue is uniquely identified by chain ID, residue number, and insertion code
struct ResidueKey {
    char chain_id;
    int residue_number;
    char insertion_code;

    bool operator==(const ResidueKey& other) const {
        return chain_id == other.chain_id &&
               residue_number == other.residue_number &&
               insertion_code == other.insertion_code;
    }
};

struct ResidueKeyHash {
    size_t operator()(const ResidueKey& k) const {
        // Combine hash of all components
        size_t h1 = std::hash<char>{}(k.chain_id);
        size_t h2 = std::hash<int>{}(k.residue_number);
        size_t h3 = std::hash<char>{}(k.insertion_code);
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};

ResidueKey GetResidueKey(const OEChem::OEAtomBase& atom) {
    const OEChem::OEResidue& res = OEChem::OEAtomGetResidue(&atom);
    return {res.GetChainID(), res.GetResidueNumber(), res.GetInsertCode()};
}
}  // namespace

ByResPredicate::ByResPredicate(Ptr child)
    : child_(std::move(child)) {}

const std::unordered_set<unsigned int>& ByResPredicate::GetMatchingResidues(Context& ctx) const {
    std::string cache_key = "byres_" + child_->ToCanonical();

    // Check if already cached
    if (ctx.HasResidueCache(cache_key)) {
        return ctx.GetResidueAtoms(cache_key);
    }

    OEChem::OEMolBase& mol = ctx.Mol();

    // First pass: find all residue keys that have at least one matching atom
    std::unordered_set<ResidueKey, ResidueKeyHash> matching_residue_keys;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
        if (child_->Evaluate(ctx, *atom)) {
            // This atom matches - get its residue key
            ResidueKey key = GetResidueKey(*atom);
            matching_residue_keys.insert(key);
        }
    }

    // Second pass: collect all atom indices that belong to matching residues
    std::unordered_set<unsigned int> matching_atom_indices;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
        ResidueKey key = GetResidueKey(*atom);
        if (matching_residue_keys.count(key) > 0) {
            matching_atom_indices.insert(atom->GetIdx());
        }
    }

    ctx.SetResidueAtoms(cache_key, std::move(matching_atom_indices));
    return ctx.GetResidueAtoms(cache_key);
}

bool ByResPredicate::Evaluate(Context& ctx, const OEChem::OEAtomBase& atom) const {
    const auto& matching = GetMatchingResidues(ctx);
    unsigned int idx = atom.GetIdx();
    return matching.count(idx) > 0;
}

std::string ByResPredicate::ToCanonical() const {
    return "byres " + child_->ToCanonical();
}

// ByChainPredicate implementation

ByChainPredicate::ByChainPredicate(Ptr child)
    : child_(std::move(child)) {}

const std::unordered_set<unsigned int>& ByChainPredicate::GetMatchingChainAtoms(Context& ctx) const {
    std::string cache_key = "bychain_" + child_->ToCanonical();

    // Check if already cached
    if (ctx.HasChainCache(cache_key)) {
        return ctx.GetChainAtoms(cache_key);
    }

    OEChem::OEMolBase& mol = ctx.Mol();

    // First pass: find all chain IDs that have at least one matching atom
    std::unordered_set<char> matching_chains;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
        if (child_->Evaluate(ctx, *atom)) {
            const OEChem::OEResidue& res = OEChem::OEAtomGetResidue(&(*atom));
            char chain_id = res.GetChainID();
            matching_chains.insert(chain_id);
        }
    }

    // Second pass: collect all atom indices that belong to matching chains
    std::unordered_set<unsigned int> matching_atom_indices;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
        const OEChem::OEResidue& res = OEChem::OEAtomGetResidue(&(*atom));
        char chain_id = res.GetChainID();
        if (matching_chains.count(chain_id) > 0) {
            matching_atom_indices.insert(atom->GetIdx());
        }
    }

    ctx.SetChainAtoms(cache_key, std::move(matching_atom_indices));
    return ctx.GetChainAtoms(cache_key);
}

bool ByChainPredicate::Evaluate(Context& ctx, const OEChem::OEAtomBase& atom) const {
    const auto& matching = GetMatchingChainAtoms(ctx);
    unsigned int idx = atom.GetIdx();
    return matching.count(idx) > 0;
}

std::string ByChainPredicate::ToCanonical() const {
    return "bychain " + child_->ToCanonical();
}

// HelixPredicate implementation
bool HelixPredicate::Evaluate(Context&, const OEChem::OEAtomBase& atom) const {
    const OEChem::OEResidue& res = OEChem::OEAtomGetResidue(&atom);
    return res.GetSecondaryStructure() == OEBio::OESecondaryStructure::Helix;
}

// SheetPredicate implementation
bool SheetPredicate::Evaluate(Context&, const OEChem::OEAtomBase& atom) const {
    const OEChem::OEResidue& res = OEChem::OEAtomGetResidue(&atom);
    return res.GetSecondaryStructure() == OEBio::OESecondaryStructure::Sheet;
}

// TurnPredicate implementation
bool TurnPredicate::Evaluate(Context&, const OEChem::OEAtomBase& atom) const {
    const OEChem::OEResidue& res = OEChem::OEAtomGetResidue(&atom);
    return res.GetSecondaryStructure() == OEBio::OESecondaryStructure::Turn;
}

// LoopPredicate implementation
bool LoopPredicate::Evaluate(Context&, const OEChem::OEAtomBase& atom) const {
    const OEChem::OEResidue& res = OEChem::OEAtomGetResidue(&atom);
    unsigned int ss = res.GetSecondaryStructure();
    // Loop/coil is anything that's not helix, sheet, or turn
    return ss != OEBio::OESecondaryStructure::Helix &&
           ss != OEBio::OESecondaryStructure::Sheet &&
           ss != OEBio::OESecondaryStructure::Turn;
}

}  // namespace OESel
