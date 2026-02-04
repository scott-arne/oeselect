// include/oeselect/Context.h
#ifndef OESELECT_CONTEXT_H
#define OESELECT_CONTEXT_H

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace OEChem {
class OEMolBase;
class OEAtomBase;
}

namespace OESel {

class OESelection;
class SpatialIndex;

/// Evaluation context with caching for a single molecule
class Context {
public:
    Context(OEChem::OEMolBase& mol, const OESelection& sele);
    ~Context();

    // Non-copyable
    Context(const Context&) = delete;
    Context& operator=(const Context&) = delete;

    /// Access molecule
    OEChem::OEMolBase& Mol();
    const OEChem::OEMolBase& Mol() const;

    /// Access selection
    const OESelection& Sele() const;

    /// Spatial index (lazy initialization)
    SpatialIndex& GetSpatialIndex();

    /// Residue membership cache
    const std::unordered_set<unsigned int>& GetResidueAtoms(const std::string& key);
    void SetResidueAtoms(const std::string& key, std::unordered_set<unsigned int> atoms);

    /// Chain membership cache
    const std::unordered_set<unsigned int>& GetChainAtoms(const std::string& key);
    void SetChainAtoms(const std::string& key, std::unordered_set<unsigned int> atoms);

    /// Distance cache
    const std::vector<bool>& GetAroundCache(const std::string& key);
    void SetAroundCache(const std::string& key, std::vector<bool> cache);
    bool HasAroundCache(const std::string& key) const;

private:
    struct Impl;
    std::unique_ptr<Impl> pimpl_;
};

}  // namespace OESel

#endif  // OESELECT_CONTEXT_H
