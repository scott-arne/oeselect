// tests/cpp/test_spatial_index.cpp
#include <gtest/gtest.h>

#include <oeselect/SpatialIndex.h>
#include <oechem.h>

#include <algorithm>
#include <cmath>

using namespace OESel;

class SpatialIndexTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a simple molecule with known coordinates
        mol_ = std::make_unique<OEChem::OEGraphMol>();
    }

    std::unique_ptr<OEChem::OEGraphMol> mol_;
};

TEST_F(SpatialIndexTest, EmptyMolecule) {
    // Empty molecule should create empty index
    SpatialIndex index(*mol_);
    EXPECT_EQ(index.Size(), 0);

    // Radius search should return empty results
    auto result = index.FindWithinRadius(0.0f, 0.0f, 0.0f, 5.0f);
    EXPECT_TRUE(result.empty());
}

TEST_F(SpatialIndexTest, SingleAtom) {
    // Create single atom at origin
    OEChem::OEAtomBase* a1 = mol_->NewAtom(6);  // Carbon
    float coords[3] = {0.0f, 0.0f, 0.0f};
    mol_->SetCoords(a1, coords);

    SpatialIndex index(*mol_);
    EXPECT_EQ(index.Size(), 1);

    // Query at origin should find the atom
    auto result = index.FindWithinRadius(0.0f, 0.0f, 0.0f, 1.0f);
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(result[0], a1->GetIdx());

    // Query far away should find nothing
    auto far_result = index.FindWithinRadius(100.0f, 0.0f, 0.0f, 1.0f);
    EXPECT_TRUE(far_result.empty());
}

TEST_F(SpatialIndexTest, FindWithinRadius) {
    // Create atoms at specific positions
    OEChem::OEAtomBase* a1 = mol_->NewAtom(6);  // C at origin
    OEChem::OEAtomBase* a2 = mol_->NewAtom(6);  // C at (1,0,0)
    OEChem::OEAtomBase* a3 = mol_->NewAtom(6);  // C at (5,0,0)

    float coords1[3] = {0.0f, 0.0f, 0.0f};
    float coords2[3] = {1.0f, 0.0f, 0.0f};
    float coords3[3] = {5.0f, 0.0f, 0.0f};

    mol_->SetCoords(a1, coords1);
    mol_->SetCoords(a2, coords2);
    mol_->SetCoords(a3, coords3);

    SpatialIndex index(*mol_);
    EXPECT_EQ(index.Size(), 3);

    // Query from origin with radius 2.0 - should find a1 and a2
    auto nearby = index.FindWithinRadius(0.0f, 0.0f, 0.0f, 2.0f);
    EXPECT_EQ(nearby.size(), 2);

    // Check that the correct atoms are found
    std::sort(nearby.begin(), nearby.end());
    EXPECT_EQ(nearby[0], a1->GetIdx());
    EXPECT_EQ(nearby[1], a2->GetIdx());

    // Query from origin with radius 10.0 - should find all atoms
    auto all = index.FindWithinRadius(0.0f, 0.0f, 0.0f, 10.0f);
    EXPECT_EQ(all.size(), 3);
}

TEST_F(SpatialIndexTest, FindWithinRadiusFromAtom) {
    // Create atoms
    OEChem::OEAtomBase* a1 = mol_->NewAtom(6);
    OEChem::OEAtomBase* a2 = mol_->NewAtom(6);
    OEChem::OEAtomBase* a3 = mol_->NewAtom(6);

    float coords1[3] = {0.0f, 0.0f, 0.0f};
    float coords2[3] = {1.5f, 0.0f, 0.0f};
    float coords3[3] = {5.0f, 0.0f, 0.0f};

    mol_->SetCoords(a1, coords1);
    mol_->SetCoords(a2, coords2);
    mol_->SetCoords(a3, coords3);

    SpatialIndex index(*mol_);

    // Query from atom a1 with radius 2.0
    auto nearby = index.FindWithinRadius(*a1, 2.0f);
    EXPECT_EQ(nearby.size(), 2);  // Should find a1 and a2
}

TEST_F(SpatialIndexTest, ExactDistanceOnBoundary) {
    // Test behavior exactly at boundary
    // Note: nanoflann uses strict less-than comparison (distance < radius)
    // so atoms exactly at the boundary may not be included
    OEChem::OEAtomBase* a1 = mol_->NewAtom(6);
    OEChem::OEAtomBase* a2 = mol_->NewAtom(6);

    float coords1[3] = {0.0f, 0.0f, 0.0f};
    float coords2[3] = {2.0f, 0.0f, 0.0f};

    mol_->SetCoords(a1, coords1);
    mol_->SetCoords(a2, coords2);

    SpatialIndex index(*mol_);

    // Radius exactly 2.0 - atom at distance 2.0 is ON the boundary
    // Use slightly larger radius to ensure inclusion
    auto result = index.FindWithinRadius(0.0f, 0.0f, 0.0f, 2.001f);
    EXPECT_EQ(result.size(), 2);

    // Verify the boundary behavior: exact radius may not include boundary atom
    auto exact = index.FindWithinRadius(0.0f, 0.0f, 0.0f, 2.0f);
    // Could be 1 or 2 depending on floating point comparisons
    EXPECT_GE(exact.size(), 1);
    EXPECT_LE(exact.size(), 2);
}

TEST_F(SpatialIndexTest, ThreeDimensionalSearch) {
    // Create atoms in 3D space
    OEChem::OEAtomBase* a1 = mol_->NewAtom(6);
    OEChem::OEAtomBase* a2 = mol_->NewAtom(6);
    OEChem::OEAtomBase* a3 = mol_->NewAtom(6);

    // a1 at origin
    // a2 at (1,1,1) - distance from origin = sqrt(3) ~ 1.732
    // a3 at (2,2,2) - distance from origin = sqrt(12) ~ 3.464
    float coords1[3] = {0.0f, 0.0f, 0.0f};
    float coords2[3] = {1.0f, 1.0f, 1.0f};
    float coords3[3] = {2.0f, 2.0f, 2.0f};

    mol_->SetCoords(a1, coords1);
    mol_->SetCoords(a2, coords2);
    mol_->SetCoords(a3, coords3);

    SpatialIndex index(*mol_);

    // Radius 2.0 should find a1 and a2 (but not a3)
    auto result = index.FindWithinRadius(0.0f, 0.0f, 0.0f, 2.0f);
    EXPECT_EQ(result.size(), 2);

    // Radius 4.0 should find all
    auto all = index.FindWithinRadius(0.0f, 0.0f, 0.0f, 4.0f);
    EXPECT_EQ(all.size(), 3);
}

TEST_F(SpatialIndexTest, NegativeCoordinates) {
    // Test with negative coordinates
    OEChem::OEAtomBase* a1 = mol_->NewAtom(6);
    OEChem::OEAtomBase* a2 = mol_->NewAtom(6);

    float coords1[3] = {-1.0f, -1.0f, -1.0f};
    float coords2[3] = {1.0f, 1.0f, 1.0f};

    mol_->SetCoords(a1, coords1);
    mol_->SetCoords(a2, coords2);

    SpatialIndex index(*mol_);

    // Query from origin - both atoms are at sqrt(3) from origin
    auto result = index.FindWithinRadius(0.0f, 0.0f, 0.0f, 2.0f);
    EXPECT_EQ(result.size(), 2);
}

TEST_F(SpatialIndexTest, LargeRadius) {
    // Test with large radius that encompasses all atoms
    for (int i = 0; i < 10; ++i) {
        OEChem::OEAtomBase* atom = mol_->NewAtom(6);
        float coords[3] = {static_cast<float>(i), 0.0f, 0.0f};
        mol_->SetCoords(atom, coords);
    }

    SpatialIndex index(*mol_);
    EXPECT_EQ(index.Size(), 10);

    // Very large radius should find all atoms
    auto result = index.FindWithinRadius(0.0f, 0.0f, 0.0f, 1000.0f);
    EXPECT_EQ(result.size(), 10);
}

TEST_F(SpatialIndexTest, SmallRadius) {
    // Test with very small radius
    OEChem::OEAtomBase* a1 = mol_->NewAtom(6);
    OEChem::OEAtomBase* a2 = mol_->NewAtom(6);

    float coords1[3] = {0.0f, 0.0f, 0.0f};
    float coords2[3] = {0.001f, 0.0f, 0.0f};

    mol_->SetCoords(a1, coords1);
    mol_->SetCoords(a2, coords2);

    SpatialIndex index(*mol_);

    // Tiny radius should find only atom at origin
    auto result = index.FindWithinRadius(0.0f, 0.0f, 0.0f, 0.0001f);
    EXPECT_EQ(result.size(), 1);
}

TEST_F(SpatialIndexTest, ZeroRadius) {
    // Test with zero radius
    // Note: nanoflann uses squared distance comparison with strict less-than
    // Zero radius means only atoms at distance 0 with dist_sq < 0 would match
    // which is never true, so zero radius returns nothing
    OEChem::OEAtomBase* a1 = mol_->NewAtom(6);
    float coords[3] = {0.0f, 0.0f, 0.0f};
    mol_->SetCoords(a1, coords);

    SpatialIndex index(*mol_);

    // Zero radius finds nothing due to strict less-than comparison
    auto result = index.FindWithinRadius(0.0f, 0.0f, 0.0f, 0.0f);
    EXPECT_EQ(result.size(), 0);

    // Tiny positive radius should find the atom at that exact position
    auto tiny = index.FindWithinRadius(0.0f, 0.0f, 0.0f, 0.001f);
    EXPECT_EQ(tiny.size(), 1);
}

TEST_F(SpatialIndexTest, ReturnsCorrectAtomIndices) {
    // Verify that returned indices match atom indices
    std::vector<unsigned int> expected_indices;
    for (int i = 0; i < 5; ++i) {
        OEChem::OEAtomBase* atom = mol_->NewAtom(6);
        float coords[3] = {static_cast<float>(i), 0.0f, 0.0f};
        mol_->SetCoords(atom, coords);
        expected_indices.push_back(atom->GetIdx());
    }

    SpatialIndex index(*mol_);

    // Find all atoms
    auto result = index.FindWithinRadius(2.0f, 0.0f, 0.0f, 100.0f);
    EXPECT_EQ(result.size(), 5);

    // Sort both for comparison
    std::sort(result.begin(), result.end());
    std::sort(expected_indices.begin(), expected_indices.end());
    EXPECT_EQ(result, expected_indices);
}
