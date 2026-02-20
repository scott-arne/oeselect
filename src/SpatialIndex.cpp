/**
 * @file SpatialIndex.cpp
 * @brief K-d tree spatial index implementation using nanoflann.
 *
 * Provides efficient O(log n) radius queries for distance-based predicates.
 */

#include "oeselect/SpatialIndex.h"

#include <oechem.h>
#include <nanoflann.hpp>

#include <memory>
#include <vector>

namespace OESel {

/**
 * @brief Point cloud adaptor for nanoflann k-d tree.
 *
 * Stores atom coordinates in a flat array format suitable for nanoflann.
 * Maintains mapping between array indices and atom indices.
 */
struct MoleculePointCloud {
    std::vector<float> coords;           ///< Flat array: x0,y0,z0,x1,y1,z1,...
    std::vector<unsigned int> atom_indices;  ///< Original atom indices

    explicit MoleculePointCloud(OEChem::OEMolBase& mol) {
        const size_t n = mol.NumAtoms();
        coords.reserve(n * 3);
        atom_indices.reserve(n);

        for (OESystem::OEIter atom = mol.GetAtoms(); atom; ++atom) {
            float xyz[3];
            const OEChem::OEAtomBase& a = *atom;
            mol.GetCoords(&a, xyz);
            coords.push_back(xyz[0]);
            coords.push_back(xyz[1]);
            coords.push_back(xyz[2]);
            atom_indices.push_back(a.GetIdx());
        }
    }

    // nanoflann required interface

    /// @brief Returns number of points in the cloud.
    [[nodiscard]] size_t kdtree_get_point_count() const { return atom_indices.size(); }

    /// @brief Returns coordinate dim of point idx.
    [[nodiscard]] float kdtree_get_pt(const size_t idx, const size_t dim) const {
        return coords[idx * 3 + dim];
    }

    /// @brief Optional bounding-box computation (disabled).
    template<class BBOX>
    bool kdtree_get_bbox(BBOX&) const { return false; }
};

/// K-d tree type using L2 (Euclidean) distance
using KDTree = nanoflann::KDTreeSingleIndexAdaptor<
    nanoflann::L2_Simple_Adaptor<float, MoleculePointCloud>,
    MoleculePointCloud,
    3               // 3D space
>;

/// PIMPL containing point cloud and k-d tree
struct SpatialIndex::Impl {
    MoleculePointCloud cloud;
    std::unique_ptr<KDTree> tree;

    explicit Impl(OEChem::OEMolBase& mol) : cloud(mol) {
        if (!cloud.atom_indices.empty()) {
            // Leaf size of 10 provides good balance between build and query time
            tree = std::make_unique<KDTree>(3, cloud, nanoflann::KDTreeSingleIndexAdaptorParams(10));
            tree->buildIndex();
        }
    }
};

SpatialIndex::SpatialIndex(OEChem::OEMolBase& mol)
    : pimpl_(std::make_unique<Impl>(mol)) {}

SpatialIndex::~SpatialIndex() = default;

std::vector<unsigned int> SpatialIndex::FindWithinRadius(const float x, const float y, const float z, const float radius) const {
    std::vector<unsigned int> result;
    if (!pimpl_->tree) return result;

    const float query[3] = {x, y, z};
    const float radius_sq = radius * radius;  // nanoflann uses squared distances

    std::vector<nanoflann::ResultItem<unsigned int, float>> matches;
    pimpl_->tree->radiusSearch(query, radius_sq, matches);

    result.reserve(matches.size());
    for (const auto& match : matches) {
        // Convert internal index back to atom index
        result.push_back(pimpl_->cloud.atom_indices[match.first]);
    }
    return result;
}

std::vector<unsigned int> SpatialIndex::FindWithinRadius(const OEChem::OEAtomBase& atom, const float radius) const {
    float xyz[3];
    atom.GetParent()->GetCoords(&atom, xyz);
    return FindWithinRadius(xyz[0], xyz[1], xyz[2], radius);
}

size_t SpatialIndex::Size() const {
    return pimpl_->cloud.atom_indices.size();
}

}  // namespace OESel
