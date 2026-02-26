/**
 * @file CustomPredicates.cpp
 * @brief OEHasResidueName and OEHasAtomNameAdvanced implementations.
 */

#include "oeselect/CustomPredicates.h"

#include <algorithm>
#include <cctype>

#include <oechem.h>

namespace {

/// Trim leading and trailing whitespace from a string.
std::string trim_whitespace(const std::string& str) {
    const auto start = str.find_first_not_of(" \t");
    if (start == std::string::npos) return "";
    const auto end = str.find_last_not_of(" \t");
    return str.substr(start, end - start + 1);
}

/// Convert a string to lowercase.
std::string to_lower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](const unsigned char c) { return std::tolower(c); });
    return result;
}

/// Normalize a name based on case and whitespace settings.
std::string normalize_name(const std::string& name,
                          const bool case_sensitive,
                          const bool whitespace) {
    std::string result = name;
    if (!case_sensitive) {
        result = to_lower(result);
    }
    if (!whitespace) {
        result = trim_whitespace(result);
    }
    return result;
}

}  // namespace

namespace OESel {

// ============================================================================
// OEHasResidueName
// ============================================================================

OEHasResidueName::OEHasResidueName(const std::string& residue_name,
                                   const bool case_sensitive,
                                   const bool whitespace)
    : residue_name_(normalize_name(residue_name, case_sensitive, whitespace)),
      case_sensitive_(case_sensitive),
      whitespace_(whitespace) {}

OEHasResidueName::OEHasResidueName(const OEHasResidueName& other)
    : residue_name_(other.residue_name_),
      case_sensitive_(other.case_sensitive_),
      whitespace_(other.whitespace_) {}

OEHasResidueName::~OEHasResidueName() = default;

bool OEHasResidueName::operator()(const OEChem::OEAtomBase& atom) const {
    const OEChem::OEResidue& res = OEChem::OEAtomGetResidue(&atom);
    const std::string resname = normalize_name(res.GetName(), case_sensitive_, whitespace_);
    return residue_name_ == resname;
}

OESystem::OEUnaryFunction<OEChem::OEAtomBase, bool>*
OEHasResidueName::CreateCopy() const {
    return new OEHasResidueName(*this);
}

// ============================================================================
// OEHasAtomNameAdvanced
// ============================================================================

OEHasAtomNameAdvanced::OEHasAtomNameAdvanced(const std::string& atom_name,
                                             const bool case_sensitive,
                                             const bool whitespace)
    : atom_name_(normalize_name(atom_name, case_sensitive, whitespace)),
      case_sensitive_(case_sensitive),
      whitespace_(whitespace) {}

OEHasAtomNameAdvanced::OEHasAtomNameAdvanced(const OEHasAtomNameAdvanced& other)
    : atom_name_(other.atom_name_),
      case_sensitive_(other.case_sensitive_),
      whitespace_(other.whitespace_) {}

OEHasAtomNameAdvanced::~OEHasAtomNameAdvanced() = default;

bool OEHasAtomNameAdvanced::operator()(const OEChem::OEAtomBase& atom) const {
    const std::string name = normalize_name(atom.GetName(), case_sensitive_, whitespace_);
    return atom_name_ == name;
}

OESystem::OEUnaryFunction<OEChem::OEAtomBase, bool>*
OEHasAtomNameAdvanced::CreateCopy() const {
    return new OEHasAtomNameAdvanced(*this);
}

}  // namespace OESel
