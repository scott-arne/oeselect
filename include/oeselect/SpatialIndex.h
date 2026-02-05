/**
 * @file SpatialIndex.h
 * @brief K-d tree spatial index for efficient distance queries.
 *
 * SpatialIndex provides O(log n) radius queries for distance-based
 * predicates (around, xaround, beyond). It uses nanoflann for the
 * underlying k-d tree implementation.
 */

#ifndef OESELECT_SPATIAL_INDEX_H
#define OESELECT_SPATIAL_INDEX_H

#include <memory>
#include <vector>

namespace OEChem {
class OEMolBase;
class OEAtomBase;
}

namespace OESel {

/**
 * @brief K-d tree based spatial index for efficient distance queries.
 *
 * This class builds a 3D k-d tree from atom coordinates to enable fast
 * radius queries. It is used internally by distance predicates (around,
 * xaround, beyond) to find atoms within a given distance.
 *
 * The index is built once on construction and is immutable. If the
 * molecule coordinates change, a new index must be created.
 *
 * @note The index stores atom positions at construction time. If molecule
 *       coordinates are modified after index creation, queries will use
 *       stale coordinate data.
 */
class SpatialIndex {
public:
    /**
     * @brief Construct spatial index from molecule coordinates.
     *
     * Builds a k-d tree from all atom positions in the molecule.
     * Construction is O(n log n) where n is the number of atoms.
     *
     * @param mol The molecule to index.
     */
    explicit SpatialIndex(OEChem::OEMolBase& mol);

    /// @brief Destructor.
    ~SpatialIndex();

    // Non-copyable (tree structure not easily copyable)
    SpatialIndex(const SpatialIndex&) = delete;
    SpatialIndex& operator=(const SpatialIndex&) = delete;

    /**
     * @brief Find all atoms within radius of a point.
     *
     * Returns atom indices for all atoms whose coordinates are within
     * the specified Euclidean distance of the query point.
     *
     * @param x X coordinate of query point.
     * @param y Y coordinate of query point.
     * @param z Z coordinate of query point.
     * @param radius Maximum distance in Angstroms.
     * @return Vector of atom indices within the radius.
     */
    std::vector<unsigned int> FindWithinRadius(float x, float y, float z, float radius) const;

    /**
     * @brief Find all atoms within radius of another atom.
     *
     * Convenience method that extracts coordinates from the atom.
     *
     * @param atom Reference atom for the query.
     * @param radius Maximum distance in Angstroms.
     * @return Vector of atom indices within the radius.
     */
    std::vector<unsigned int> FindWithinRadius(const OEChem::OEAtomBase& atom, float radius) const;

    /**
     * @brief Get the number of atoms in the index.
     * @return Number of indexed atoms.
     */
    size_t Size() const;

private:
    struct Impl;
    std::unique_ptr<Impl> pimpl_;  ///< PIMPL containing k-d tree
};

}  // namespace OESel

#endif  // OESELECT_SPATIAL_INDEX_H
