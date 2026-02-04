// tests/cpp/test_selection.cpp
#include <gtest/gtest.h>

#include <oeselect/oeselect.h>
#include <oechem.h>

using namespace OESel;

class SelectionTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a simple molecule for testing
        OEChem::OESmilesToMol(mol_, "CC(=O)OC1=CC=CC=C1C(=O)O");  // Aspirin
        OEChem::OEAssignAromaticFlags(mol_);
    }

    OEChem::OEGraphMol mol_;
};

TEST_F(SelectionTest, EmptySelectionMatchesAll) {
    OESelection sele = OESelection::Parse("");
    EXPECT_TRUE(sele.IsEmpty());
    EXPECT_EQ(sele.ToCanonical(), "all");
}

TEST_F(SelectionTest, OESelectMatchesAllAtoms) {
    OESelection sele = OESelection::Parse("");
    OESelect sel(mol_, sele);

    int count = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol_.GetAtoms(); atom; ++atom) {
        const OEChem::OEAtomBase& a = *atom;
        if (sel(a)) {
            count++;
        }
    }
    EXPECT_EQ(count, static_cast<int>(mol_.NumAtoms()));
}

TEST_F(SelectionTest, NamePredicateExact) {
    // First, set up atom names on the molecule
    int idx = 1;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol_.GetAtoms(); atom; ++atom) {
        atom->SetName(("C" + std::to_string(idx++)).c_str());
    }

    OESelect sel(mol_, "name C1");

    int count = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol_.GetAtoms(); atom; ++atom) {
        if (sel(*atom)) {
            count++;
        }
    }
    EXPECT_EQ(count, 1);  // Only one atom named C1
}

TEST_F(SelectionTest, NamePredicateWildcard) {
    // Set up atom names
    int idx = 1;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol_.GetAtoms(); atom; ++atom) {
        atom->SetName(("C" + std::to_string(idx++)).c_str());
    }

    OESelect sel(mol_, "name C*");

    int count = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol_.GetAtoms(); atom; ++atom) {
        if (sel(*atom)) {
            count++;
        }
    }
    EXPECT_GT(count, 0);  // Multiple atoms match C*
    EXPECT_EQ(count, static_cast<int>(mol_.NumAtoms()));  // All atoms start with C
}

// Logical operator tests

TEST_F(SelectionTest, AndOperator) {
    // Set up atom names like C1, C2, C10, C11, etc.
    int idx = 1;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol_.GetAtoms(); atom; ++atom) {
        atom->SetName(("C" + std::to_string(idx++)).c_str());
    }

    // Find atoms whose name starts with C AND ends with 1
    OESelect sel(mol_, "name C* and name *1");

    int count = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol_.GetAtoms(); atom; ++atom) {
        if (sel(*atom)) {
            count++;
        }
    }
    // C1, C11 should match (atoms ending in 1)
    EXPECT_GT(count, 0);
}

TEST_F(SelectionTest, OrOperator) {
    // Set up atom names
    int idx = 1;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol_.GetAtoms(); atom; ++atom) {
        if (idx == 1) {
            atom->SetName("O1");
        } else if (idx == 2) {
            atom->SetName("O2");
        } else {
            atom->SetName(("A" + std::to_string(idx)).c_str());
        }
        idx++;
    }

    OESelect sel(mol_, "name O1 or name O2");

    int count = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol_.GetAtoms(); atom; ++atom) {
        if (sel(*atom)) {
            count++;
        }
    }
    EXPECT_EQ(count, 2);  // O1 and O2 both match
}

TEST_F(SelectionTest, NotOperator) {
    // Set up atom names - some starting with C, some with O
    int idx = 1;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol_.GetAtoms(); atom; ++atom) {
        if (idx <= 5) {
            atom->SetName(("C" + std::to_string(idx)).c_str());
        } else {
            atom->SetName(("O" + std::to_string(idx)).c_str());
        }
        idx++;
    }

    OESelect all(mol_, "");
    OESelect notC(mol_, "not name C*");

    int all_count = 0, not_c_count = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol_.GetAtoms(); atom; ++atom) {
        if (all(*atom)) all_count++;
        if (notC(*atom)) not_c_count++;
    }
    EXPECT_LT(not_c_count, all_count);
    EXPECT_EQ(not_c_count, all_count - 5);  // 5 atoms start with C
}

TEST_F(SelectionTest, XorOperator) {
    // Set up atom names
    int idx = 1;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol_.GetAtoms(); atom; ++atom) {
        if (idx == 1) {
            atom->SetName("AB1");  // Matches both A* and *B*
        } else if (idx == 2) {
            atom->SetName("X1");   // Matches neither
        } else {
            atom->SetName(("A" + std::to_string(idx)).c_str());
        }
        idx++;
    }

    // XOR: match A* OR *B* but not both
    OESelect sel(mol_, "name A* xor name *B*");

    int count = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol_.GetAtoms(); atom; ++atom) {
        if (sel(*atom)) {
            count++;
        }
    }
    // AB1 matches both, so XOR should NOT match it
    // A3, A4, etc. match only A*, so XOR should match them
    EXPECT_GT(count, 0);
}

TEST_F(SelectionTest, ParenthesesGrouping) {
    // Set up atom names
    int idx = 1;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol_.GetAtoms(); atom; ++atom) {
        if (idx == 1) {
            atom->SetName("X1");
        } else if (idx == 2) {
            atom->SetName("Y1");
        } else if (idx == 3) {
            atom->SetName("Z1");
        } else {
            atom->SetName(("A" + std::to_string(idx)).c_str());
        }
        idx++;
    }

    // (X* or Y*) and *1 should match X1 and Y1
    OESelect sel(mol_, "(name X* or name Y*) and name *1");

    int count = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol_.GetAtoms(); atom; ++atom) {
        if (sel(*atom)) {
            count++;
        }
    }
    EXPECT_EQ(count, 2);  // X1 and Y1
}

TEST_F(SelectionTest, NestedLogicalOperators) {
    // Set up atom names
    int idx = 1;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol_.GetAtoms(); atom; ++atom) {
        atom->SetName(("C" + std::to_string(idx++)).c_str());
    }

    // not (name C1 or name C2) should exclude C1 and C2
    OESelect sel(mol_, "not (name C1 or name C2)");

    int count = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol_.GetAtoms(); atom; ++atom) {
        if (sel(*atom)) {
            count++;
        }
    }
    // Total atoms minus C1 and C2
    EXPECT_EQ(count, static_cast<int>(mol_.NumAtoms()) - 2);
}

TEST_F(SelectionTest, OperatorPrecedence) {
    // Set up atom names
    int idx = 1;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol_.GetAtoms(); atom; ++atom) {
        if (idx == 1) {
            atom->SetName("X1");
        } else if (idx == 2) {
            atom->SetName("Y1");
        } else if (idx == 3) {
            atom->SetName("X2");
        } else {
            atom->SetName(("A" + std::to_string(idx)).c_str());
        }
        idx++;
    }

    // "name X* or name Y* and name *1" should parse as "name X* or (name Y* and name *1)"
    // due to AND having higher precedence than OR
    // This should match: X1, X2 (from X*), Y1 (from Y* and *1)
    OESelect sel(mol_, "name X* or name Y* and name *1");

    int count = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol_.GetAtoms(); atom; ++atom) {
        if (sel(*atom)) {
            count++;
        }
    }
    EXPECT_EQ(count, 3);  // X1, X2, Y1
}
