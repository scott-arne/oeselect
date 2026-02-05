// src/Context.cpp
#include "oeselect/Context.h"
#include "oeselect/Selection.h"
#include "oeselect/SpatialIndex.h"

#include <oechem.h>

namespace OESel {

struct Context::Impl {
    OEChem::OEMolBase& mol;
    const OESelection& sele;
    std::unique_ptr<SpatialIndex> spatial_index;
    std::unordered_map<std::string, std::unordered_set<unsigned int>> residue_cache;
    std::unordered_map<std::string, std::unordered_set<unsigned int>> chain_cache;
    std::unordered_map<std::string, std::vector<bool>> around_cache;

    Impl(OEChem::OEMolBase& m, const OESelection& s) : mol(m), sele(s) {}
};

Context::Context(OEChem::OEMolBase& mol, const OESelection& sele)
    : pimpl_(std::make_unique<Impl>(mol, sele)) {}

Context::~Context() = default;

OEChem::OEMolBase& Context::Mol() { return pimpl_->mol; }
const OEChem::OEMolBase& Context::Mol() const { return pimpl_->mol; }
const OESelection& Context::Sele() const { return pimpl_->sele; }

SpatialIndex& Context::GetSpatialIndex() {
    if (!pimpl_->spatial_index) {
        pimpl_->spatial_index = std::make_unique<SpatialIndex>(pimpl_->mol);
    }
    return *pimpl_->spatial_index;
}

const std::unordered_set<unsigned int>& Context::GetResidueAtoms(const std::string& key) {
    return pimpl_->residue_cache[key];
}

void Context::SetResidueAtoms(const std::string& key, std::unordered_set<unsigned int> atoms) {
    pimpl_->residue_cache[key] = std::move(atoms);
}

bool Context::HasResidueCache(const std::string& key) const {
    return pimpl_->residue_cache.find(key) != pimpl_->residue_cache.end();
}

const std::unordered_set<unsigned int>& Context::GetChainAtoms(const std::string& key) {
    return pimpl_->chain_cache[key];
}

void Context::SetChainAtoms(const std::string& key, std::unordered_set<unsigned int> atoms) {
    pimpl_->chain_cache[key] = std::move(atoms);
}

bool Context::HasChainCache(const std::string& key) const {
    return pimpl_->chain_cache.find(key) != pimpl_->chain_cache.end();
}

const std::vector<bool>& Context::GetAroundCache(const std::string& key) {
    return pimpl_->around_cache[key];
}

void Context::SetAroundCache(const std::string& key, std::vector<bool> cache) {
    pimpl_->around_cache[key] = std::move(cache);
}

bool Context::HasAroundCache(const std::string& key) const {
    return pimpl_->around_cache.find(key) != pimpl_->around_cache.end();
}

}  // namespace OESel
