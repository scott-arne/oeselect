// src/SpatialIndex.cpp
#include "oeselect/SpatialIndex.h"

#include <oechem.h>
#include <nanoflann.hpp>

#include <memory>
#include <vector>

namespace OESel {

/// Point cloud adaptor for nanoflann k-d tree
struct MoleculePointCloud {
    std::vector<float> coords;  // x, y, z for each atom
    std::vector<unsigned int> atom_indices;

    explicit MoleculePointCloud(OEChem::OEMolBase& mol) {
        size_t n = mol.NumAtoms();
        coords.reserve(n * 3);
        atom_indices.reserve(n);

        for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
            float xyz[3];
            const OEChem::OEAtomBase& a = *atom;
            mol.GetCoords(&a, xyz);
            coords.push_back(xyz[0]);
            coords.push_back(xyz[1]);
            coords.push_back(xyz[2]);
            atom_indices.push_back(a.GetIdx());
        }
    }

    // nanoflann interface methods

    /// Returns number of points in the cloud
    size_t kdtree_get_point_count() const { return atom_indices.size(); }

    /// Returns the coordinate of point idx in dimension dim
    float kdtree_get_pt(size_t idx, size_t dim) const {
        return coords[idx * 3 + dim];
    }

    /// Optional bounding-box computation (returns false to skip)
    template<class BBOX>
    bool kdtree_get_bbox(BBOX&) const { return false; }
};

using KDTree = nanoflann::KDTreeSingleIndexAdaptor<
    nanoflann::L2_Simple_Adaptor<float, MoleculePointCloud>,
    MoleculePointCloud,
    3,  // dimensions
    unsigned int  // index type
>;

struct SpatialIndex::Impl {
    MoleculePointCloud cloud;
    std::unique_ptr<KDTree> tree;

    explicit Impl(OEChem::OEMolBase& mol) : cloud(mol) {
        if (!cloud.atom_indices.empty()) {
            tree = std::make_unique<KDTree>(3, cloud, nanoflann::KDTreeSingleIndexAdaptorParams(10));
            tree->buildIndex();
        }
    }
};

SpatialIndex::SpatialIndex(OEChem::OEMolBase& mol)
    : pimpl_(std::make_unique<Impl>(mol)) {}

SpatialIndex::~SpatialIndex() = default;

std::vector<unsigned int> SpatialIndex::FindWithinRadius(float x, float y, float z, float radius) const {
    std::vector<unsigned int> result;
    if (!pimpl_->tree) return result;

    float query[3] = {x, y, z};
    float radius_sq = radius * radius;

    std::vector<nanoflann::ResultItem<unsigned int, float>> matches;
    pimpl_->tree->radiusSearch(query, radius_sq, matches);

    result.reserve(matches.size());
    for (const auto& match : matches) {
        result.push_back(pimpl_->cloud.atom_indices[match.first]);
    }
    return result;
}

std::vector<unsigned int> SpatialIndex::FindWithinRadius(const OEChem::OEAtomBase& atom, float radius) const {
    float xyz[3];
    atom.GetParent()->GetCoords(&atom, xyz);
    return FindWithinRadius(xyz[0], xyz[1], xyz[2], radius);
}

size_t SpatialIndex::Size() const {
    return pimpl_->cloud.atom_indices.size();
}

}  // namespace OESel
