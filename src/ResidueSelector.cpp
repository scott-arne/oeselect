/**
 * @file ResidueSelector.cpp
 * @brief Selector, OEResidueSelector, and utility function implementations.
 */

#include "oeselect/ResidueSelector.h"
#include "oeselect/Error.h"
#include "oeselect/Selection.h"
#include "oeselect/Selector.h"

#include <regex>
#include <sstream>
#include <tuple>

#include <oechem.h>

namespace OESel {

// ============================================================================
// Selector
// ============================================================================

Selector Selector::FromAtom(const OEChem::OEAtomBase& atom) {
    const OEChem::OEResidue res = OEChem::OEAtomGetResidue(&atom);
    return FromResidue(res);
}

Selector Selector::FromResidue(const OEChem::OEResidue& res) {
    return {
        res.GetName(),
        res.GetResidueNumber(),
        std::string(1, res.GetChainID()),
        std::string(1, res.GetInsertCode())
    };
}

Selector Selector::FromString(const std::string& selector_str) {
    // Parse "NAME:NUMBER:ICODE:CHAIN" format
    std::vector<std::string> parts;
    std::istringstream stream(selector_str);
    std::string part;
    while (std::getline(stream, part, ':')) {
        parts.push_back(part);
    }

    if (parts.size() != 4) {
        throw SelectionError("Invalid selector format: " + selector_str +
                             " (expected NAME:NUMBER:ICODE:CHAIN)");
    }

    // Trim whitespace from parts
    for (auto& p : parts) {
        // Trim leading whitespace
        if (auto start = p.find_first_not_of(" \t"); start != std::string::npos) {
            p = p.substr(start);
        }
        // Trim trailing whitespace
        if (auto end = p.find_last_not_of(" \t"); end != std::string::npos) {
            p = p.substr(0, end + 1);
        }
    }

    int residue_number;
    try {
        residue_number = std::stoi(parts[1]);
    } catch (const std::exception&) {
        throw SelectionError("Invalid residue number in selector: " + parts[1]);
    }

    return {parts[0], residue_number, parts[3], parts[2]};
}

std::string Selector::ToString() const {
    return name + ":" + std::to_string(residue_number) + ":" +
           insert_code + ":" + chain;
}

bool Selector::operator<(const Selector& other) const {
    return std::tie(chain, residue_number, insert_code) <
           std::tie(other.chain, other.residue_number, other.insert_code);
}

bool Selector::operator>(const Selector& other) const {
    return other < *this;
}

bool Selector::operator<=(const Selector& other) const {
    return !(other < *this);
}

bool Selector::operator>=(const Selector& other) const {
    return !(*this < other);
}

bool Selector::operator==(const Selector& other) const {
    return std::tie(name, residue_number, chain, insert_code) ==
           std::tie(other.name, other.residue_number, other.chain, other.insert_code);
}

bool Selector::operator!=(const Selector& other) const {
    return !(*this == other);
}

// ============================================================================
// OEResidueSelector
// ============================================================================

OEResidueSelector::OEResidueSelector(const std::string& selector_str)
    : selectors_(ParseSelectorSet(selector_str)) {}

OEResidueSelector::OEResidueSelector(const std::set<Selector>& selectors)
    : selectors_(selectors) {}

OEResidueSelector::OEResidueSelector(const OEResidueSelector& other)
    : selectors_(other.selectors_) {}

OEResidueSelector::~OEResidueSelector() = default;

bool OEResidueSelector::operator()(const OEChem::OEAtomBase& atom) const {
    const Selector sel = Selector::FromAtom(atom);
    return selectors_.count(sel) > 0;
}

OESystem::OEUnaryFunction<OEChem::OEAtomBase, bool>*
OEResidueSelector::CreateCopy() const {
    return new OEResidueSelector(*this);
}

// ============================================================================
// Utility functions
// ============================================================================

std::set<Selector> ParseSelectorSet(const std::string& selector_str) {
    std::set<Selector> result;
    // Split on comma, semicolon, ampersand, tab, and newline
    static const std::regex delimiter(R"([,;&\t\n]+)");

    std::sregex_token_iterator it(selector_str.begin(), selector_str.end(),
                                  delimiter, -1);
    for (std::sregex_token_iterator end; it != end; ++it) {
        std::string token = it->str();
        // Trim whitespace
        auto start = token.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) continue;
        auto finish = token.find_last_not_of(" \t\r\n");
        token = token.substr(start, finish - start + 1);

        if (!token.empty()) {
            result.insert(Selector::FromString(token));
        }
    }
    return result;
}

std::set<Selector> MolToSelectorSet(const OEChem::OEMolBase& mol) {
    std::set<Selector> result;
    for (OESystem::OEIter atom = mol.GetAtoms(); atom; ++atom) {
        result.insert(Selector::FromAtom(*atom));
    }
    return result;
}

std::set<std::string> StrSelectorSet(
        OEChem::OEMolBase& mol, const std::string& selection_str) {
    const OESelection sele = OESelection::Parse(selection_str);
    const OESelect selector(mol, sele);

    std::set<std::string> result;
    for (OESystem::OEIter atom = mol.GetAtoms(); atom; ++atom) {
        if (selector(*atom)) {
            result.insert(GetSelectorString(*atom));
        }
    }
    return result;
}

std::string GetSelectorString(const OEChem::OEAtomBase& atom) {
    return Selector::FromAtom(atom).ToString();
}

}  // namespace OESel
