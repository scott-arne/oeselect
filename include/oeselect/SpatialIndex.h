// include/oeselect/SpatialIndex.h
#ifndef OESELECT_SPATIAL_INDEX_H
#define OESELECT_SPATIAL_INDEX_H

#include <memory>
#include <vector>

namespace OEChem {
class OEMolBase;
class OEAtomBase;
}

namespace OESel {

/// K-d tree based spatial index for efficient distance queries
class SpatialIndex {
public:
    explicit SpatialIndex(OEChem::OEMolBase& mol);
    ~SpatialIndex();

    // Non-copyable
    SpatialIndex(const SpatialIndex&) = delete;
    SpatialIndex& operator=(const SpatialIndex&) = delete;

    /// Find all atom indices within radius of a point
    std::vector<unsigned int> FindWithinRadius(float x, float y, float z, float radius) const;

    /// Find all atom indices within radius of an atom
    std::vector<unsigned int> FindWithinRadius(const OEChem::OEAtomBase& atom, float radius) const;

    /// Get total number of atoms in index
    size_t Size() const;

private:
    struct Impl;
    std::unique_ptr<Impl> pimpl_;
};

}  // namespace OESel

#endif  // OESELECT_SPATIAL_INDEX_H
