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

// ============================================================================
// Special Keywords (all, none) Tests - Phase 4
// ============================================================================

TEST_F(SelectionTest, AllKeywordMatchesAllAtoms) {
    OESelect sel(mol_, "all");

    int count = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol_.GetAtoms(); atom; ++atom) {
        if (sel(*atom)) {
            count++;
        }
    }
    EXPECT_EQ(count, static_cast<int>(mol_.NumAtoms()));
}

TEST_F(SelectionTest, NoneKeywordMatchesNoAtoms) {
    OESelect sel(mol_, "none");

    int count = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol_.GetAtoms(); atom; ++atom) {
        if (sel(*atom)) {
            count++;
        }
    }
    EXPECT_EQ(count, 0);
}

TEST_F(SelectionTest, AllKeywordInLogicalExpression) {
    // Set up atom names
    int idx = 1;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol_.GetAtoms(); atom; ++atom) {
        atom->SetName(("C" + std::to_string(idx++)).c_str());
    }

    // "all and name C1" should only match C1
    OESelect sel(mol_, "all and name C1");

    int count = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol_.GetAtoms(); atom; ++atom) {
        if (sel(*atom)) {
            count++;
        }
    }
    EXPECT_EQ(count, 1);
}

TEST_F(SelectionTest, NoneKeywordInLogicalExpression) {
    // Set up atom names
    int idx = 1;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol_.GetAtoms(); atom; ++atom) {
        atom->SetName(("C" + std::to_string(idx++)).c_str());
    }

    // "none or name C1" should match C1
    OESelect sel(mol_, "none or name C1");

    int count = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol_.GetAtoms(); atom; ++atom) {
        if (sel(*atom)) {
            count++;
        }
    }
    EXPECT_EQ(count, 1);
}

// ============================================================================
// Multi-value Syntax Tests - Phase 4
// ============================================================================

TEST_F(SelectionTest, MultiValueNameSyntax) {
    // Set up atom names
    int idx = 1;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol_.GetAtoms(); atom; ++atom) {
        atom->SetName(("C" + std::to_string(idx++)).c_str());
    }

    // "name C1+C2+C3" should match atoms named C1, C2, or C3
    OESelect sel(mol_, "name C1+C2+C3");

    int count = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol_.GetAtoms(); atom; ++atom) {
        if (sel(*atom)) {
            count++;
        }
    }
    EXPECT_EQ(count, 3);
}

TEST_F(SelectionTest, MultiValueNameWithLogical) {
    // Set up atom names
    int idx = 1;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol_.GetAtoms(); atom; ++atom) {
        atom->SetName(("C" + std::to_string(idx++)).c_str());
    }

    // "name C1+C2 and name *1" should match C1 (has 1 and is in C1+C2 list)
    OESelect sel(mol_, "name C1+C2 and name *1");

    int count = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol_.GetAtoms(); atom; ++atom) {
        if (sel(*atom)) {
            count++;
        }
    }
    EXPECT_EQ(count, 1);  // Only C1
}

// ============================================================================
// Hierarchical Macro Syntax Tests - Phase 4
// ============================================================================

class MacroSyntaxTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a protein-like structure with residue info
        OEChem::OESmilesToMol(mol_, "CC(N)C(=O)NCC(=O)O");  // Simplified peptide

        // Set up atoms with names, residues, and chains
        int atomIdx = 0;
        for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol_.GetAtoms(); atom; ++atom) {
            OEChem::OEResidue res;

            if (atomIdx < 3) {
                // First 3 atoms: ALA, residue 1, chain A, names CA, CB, N
                res.SetName("ALA");
                res.SetResidueNumber(1);
                res.SetChainID('A');
                if (atomIdx == 0) atom->SetName("CA");
                else if (atomIdx == 1) atom->SetName("CB");
                else atom->SetName("N");
            } else if (atomIdx < 5) {
                // Next 2 atoms: ALA, residue 1, chain A, names C, O
                res.SetName("ALA");
                res.SetResidueNumber(1);
                res.SetChainID('A');
                if (atomIdx == 3) atom->SetName("C");
                else atom->SetName("O");
            } else {
                // Remaining atoms: GLY, residue 2, chain B
                res.SetName("GLY");
                res.SetResidueNumber(2);
                res.SetChainID('B');
                if (atomIdx == 5) atom->SetName("CA");
                else if (atomIdx == 6) atom->SetName("N");
                else atom->SetName("C");
            }
            OEChem::OEAtomSetResidue(&(*atom), res);
            atomIdx++;
        }
    }

    OEChem::OEGraphMol mol_;
};

TEST_F(MacroSyntaxTest, MacroChainOnly) {
    // //A// selects all atoms in chain A
    OESelect sel(mol_, "//A//");

    int count = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol_.GetAtoms(); atom; ++atom) {
        if (sel(*atom)) {
            count++;
        }
    }
    EXPECT_EQ(count, 5);  // 5 atoms in chain A
}

TEST_F(MacroSyntaxTest, MacroChainAndResi) {
    // //A/1/ selects all atoms in chain A, residue 1
    OESelect sel(mol_, "//A/1/");

    int count = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol_.GetAtoms(); atom; ++atom) {
        if (sel(*atom)) {
            count++;
        }
    }
    EXPECT_EQ(count, 5);  // All chain A atoms are residue 1
}

TEST_F(MacroSyntaxTest, MacroChainResiName) {
    // //A/1/CA selects CA atom in chain A, residue 1
    OESelect sel(mol_, "//A/1/CA");

    int count = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol_.GetAtoms(); atom; ++atom) {
        if (sel(*atom)) {
            count++;
        }
    }
    EXPECT_EQ(count, 1);  // Only one CA in chain A, residue 1
}

TEST_F(MacroSyntaxTest, MacroNameOnly) {
    // ////CA selects all CA atoms regardless of chain/residue (//chain/resi/name format)
    OESelect sel(mol_, "////CA");

    int count = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol_.GetAtoms(); atom; ++atom) {
        if (sel(*atom)) {
            count++;
        }
    }
    EXPECT_EQ(count, 2);  // CA in chain A and CA in chain B
}

TEST_F(MacroSyntaxTest, MacroAllWildcard) {
    // //// selects all atoms (all wildcards)
    OESelect sel(mol_, "////");

    int count = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol_.GetAtoms(); atom; ++atom) {
        if (sel(*atom)) {
            count++;
        }
    }
    EXPECT_EQ(count, static_cast<int>(mol_.NumAtoms()));
}

TEST_F(MacroSyntaxTest, MacroWithLogical) {
    // //A// or //B// should select all atoms
    OESelect sel(mol_, "//A// or //B//");

    int count = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol_.GetAtoms(); atom; ++atom) {
        if (sel(*atom)) {
            count++;
        }
    }
    EXPECT_EQ(count, static_cast<int>(mol_.NumAtoms()));
}

// ============================================================================
// Secondary Structure Predicate Tests - Phase 4
// ============================================================================

class SecondaryStructureTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a protein-like structure with secondary structure info
        OEChem::OESmilesToMol(mol_, "CC(N)C(=O)NCC(=O)O");  // Simplified peptide

        // Set up atoms with secondary structure
        int atomIdx = 0;
        for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol_.GetAtoms(); atom; ++atom) {
            OEChem::OEResidue res;
            res.SetName("ALA");
            res.SetResidueNumber(atomIdx / 3 + 1);
            res.SetChainID('A');

            // First 3 atoms: helix
            // Next 3 atoms: sheet
            // Remaining: loop (default/unset)
            if (atomIdx < 3) {
                res.SetSecondaryStructure(OEBio::OESecondaryStructure::Helix);
            } else if (atomIdx < 6) {
                res.SetSecondaryStructure(OEBio::OESecondaryStructure::Sheet);
            }
            // else: leave unset for loop
            OEChem::OEAtomSetResidue(&(*atom), res);
            atomIdx++;
        }
    }

    OEChem::OEGraphMol mol_;
};

TEST_F(SecondaryStructureTest, HelixPredicate) {
    OESelect sel(mol_, "helix");

    int count = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol_.GetAtoms(); atom; ++atom) {
        if (sel(*atom)) {
            count++;
        }
    }
    EXPECT_EQ(count, 3);  // First 3 atoms have helix secondary structure
}

TEST_F(SecondaryStructureTest, SheetPredicate) {
    OESelect sel(mol_, "sheet");

    int count = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol_.GetAtoms(); atom; ++atom) {
        if (sel(*atom)) {
            count++;
        }
    }
    EXPECT_EQ(count, 3);  // Atoms 4-6 have sheet secondary structure
}

TEST_F(SecondaryStructureTest, LoopPredicate) {
    OESelect sel(mol_, "loop");

    int count = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol_.GetAtoms(); atom; ++atom) {
        if (sel(*atom)) {
            count++;
        }
    }
    // Loop is anything not helix, sheet, or turn
    int total = static_cast<int>(mol_.NumAtoms());
    EXPECT_EQ(count, total - 6);  // 6 atoms are helix or sheet
}

TEST_F(SecondaryStructureTest, SecondaryStructureWithLogical) {
    OESelect sel(mol_, "helix or sheet");

    int count = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol_.GetAtoms(); atom; ++atom) {
        if (sel(*atom)) {
            count++;
        }
    }
    EXPECT_EQ(count, 6);  // helix (3) + sheet (3)
}

// ============================================================================
// Atom Property Predicate Tests (Task 8)
// ============================================================================

class AtomPropertyTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a small protein-like structure with residue info
        // Using a simple dipeptide structure: Ala-Gly
        OEChem::OESmilesToMol(mol_, "CC(N)C(=O)NCC(=O)O");  // Simplified peptide

        // Set up residue information manually
        int atomIdx = 0;
        for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol_.GetAtoms(); atom; ++atom) {
            OEChem::OEResidue res;

            // First 5 atoms are ALA (residue 1, chain A), rest are GLY (residue 2, chain A)
            if (atomIdx < 5) {
                res.SetName("ALA");
                res.SetResidueNumber(1);
                res.SetChainID('A');
            } else {
                res.SetName("GLY");
                res.SetResidueNumber(2);
                res.SetChainID('A');
            }
            OEChem::OEAtomSetResidue(&(*atom), res);
            atomIdx++;
        }
    }

    OEChem::OEGraphMol mol_;
};

TEST_F(AtomPropertyTest, ResnPredicateExact) {
    OESelect sel(mol_, "resn ALA");

    int count = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol_.GetAtoms(); atom; ++atom) {
        if (sel(*atom)) {
            count++;
        }
    }
    EXPECT_EQ(count, 5);  // 5 atoms in ALA residue
}

TEST_F(AtomPropertyTest, ResnPredicateWildcard) {
    OESelect sel(mol_, "resn GL*");

    int count = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol_.GetAtoms(); atom; ++atom) {
        if (sel(*atom)) {
            count++;
        }
    }
    // GLY should match GL* pattern
    int total = static_cast<int>(mol_.NumAtoms());
    EXPECT_EQ(count, total - 5);  // All atoms except the 5 in ALA
}

TEST_F(AtomPropertyTest, ResiPredicateExact) {
    OESelect sel(mol_, "resi 1");

    int count = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol_.GetAtoms(); atom; ++atom) {
        if (sel(*atom)) {
            count++;
        }
    }
    EXPECT_EQ(count, 5);  // 5 atoms in residue 1 (ALA)
}

TEST_F(AtomPropertyTest, ResiPredicateRange) {
    OESelect sel(mol_, "resi 1-2");

    int count = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol_.GetAtoms(); atom; ++atom) {
        if (sel(*atom)) {
            count++;
        }
    }
    // Both residues 1 and 2 should be selected
    EXPECT_EQ(count, static_cast<int>(mol_.NumAtoms()));
}

TEST_F(AtomPropertyTest, ResiPredicateGreaterThan) {
    OESelect sel(mol_, "resi > 1");

    int count = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol_.GetAtoms(); atom; ++atom) {
        if (sel(*atom)) {
            count++;
        }
    }
    // Only residue 2 (GLY) should match
    int total = static_cast<int>(mol_.NumAtoms());
    EXPECT_EQ(count, total - 5);
}

TEST_F(AtomPropertyTest, ResiPredicateLessThanOrEqual) {
    OESelect sel(mol_, "resi <= 1");

    int count = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol_.GetAtoms(); atom; ++atom) {
        if (sel(*atom)) {
            count++;
        }
    }
    // Only residue 1 (ALA) should match
    EXPECT_EQ(count, 5);
}

TEST_F(AtomPropertyTest, ChainPredicate) {
    OESelect sel(mol_, "chain A");

    int count = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol_.GetAtoms(); atom; ++atom) {
        if (sel(*atom)) {
            count++;
        }
    }
    // All atoms are in chain A
    EXPECT_EQ(count, static_cast<int>(mol_.NumAtoms()));
}

TEST_F(AtomPropertyTest, ChainPredicateNoMatch) {
    OESelect sel(mol_, "chain B");

    int count = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol_.GetAtoms(); atom; ++atom) {
        if (sel(*atom)) {
            count++;
        }
    }
    // No atoms are in chain B
    EXPECT_EQ(count, 0);
}

// Use the aspirin molecule for element tests (has C, O, and H atoms)
class ElemTest : public ::testing::Test {
protected:
    void SetUp() override {
        OEChem::OESmilesToMol(mol_, "CC(=O)OC1=CC=CC=C1C(=O)O");  // Aspirin
    }

    OEChem::OEGraphMol mol_;
};

TEST_F(ElemTest, ElemPredicateCarbon) {
    OESelect sel(mol_, "elem C");

    int count = 0;
    int expected_carbons = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol_.GetAtoms(); atom; ++atom) {
        if (atom->GetAtomicNum() == 6) {
            expected_carbons++;
        }
        if (sel(*atom)) {
            count++;
        }
    }
    EXPECT_EQ(count, expected_carbons);
    EXPECT_GT(count, 0);  // Should have some carbon atoms
}

TEST_F(ElemTest, ElemPredicateOxygen) {
    OESelect sel(mol_, "elem O");

    int count = 0;
    int expected_oxygens = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol_.GetAtoms(); atom; ++atom) {
        if (atom->GetAtomicNum() == 8) {
            expected_oxygens++;
        }
        if (sel(*atom)) {
            count++;
        }
    }
    EXPECT_EQ(count, expected_oxygens);
    EXPECT_GT(count, 0);  // Should have some oxygen atoms
}

TEST_F(ElemTest, ElemPredicateCaseInsensitive) {
    OESelect sel_lower(mol_, "elem c");
    OESelect sel_upper(mol_, "elem C");

    int count_lower = 0;
    int count_upper = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol_.GetAtoms(); atom; ++atom) {
        if (sel_lower(*atom)) count_lower++;
        if (sel_upper(*atom)) count_upper++;
    }
    EXPECT_EQ(count_lower, count_upper);
    EXPECT_GT(count_lower, 0);
}

TEST_F(SelectionTest, IndexPredicateExact) {
    OESelect sel(mol_, "index 0");

    int count = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol_.GetAtoms(); atom; ++atom) {
        if (sel(*atom)) {
            count++;
        }
    }
    EXPECT_EQ(count, 1);  // Only one atom with index 0
}

TEST_F(SelectionTest, IndexPredicateRange) {
    OESelect sel(mol_, "index 0-4");

    int count = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol_.GetAtoms(); atom; ++atom) {
        if (sel(*atom)) {
            count++;
        }
    }
    EXPECT_EQ(count, 5);  // Atoms 0, 1, 2, 3, 4
}

TEST_F(SelectionTest, IndexPredicateLargeValue) {
    // Test with an index that doesn't exist
    OESelect sel(mol_, "index 9999");

    int count = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol_.GetAtoms(); atom; ++atom) {
        if (sel(*atom)) {
            count++;
        }
    }
    EXPECT_EQ(count, 0);  // No atom with index 9999
}

TEST_F(SelectionTest, IndexPredicateGreaterThan) {
    OESelect sel(mol_, "index > 5");

    int count = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol_.GetAtoms(); atom; ++atom) {
        if (sel(*atom)) {
            count++;
        }
    }
    // Atoms with index 6, 7, 8, ... should match
    int total = static_cast<int>(mol_.NumAtoms());
    EXPECT_EQ(count, total - 6);  // All atoms except 0-5
}

TEST_F(SelectionTest, IndexPredicateLessThanOrEqual) {
    OESelect sel(mol_, "index <= 3");

    int count = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol_.GetAtoms(); atom; ++atom) {
        if (sel(*atom)) {
            count++;
        }
    }
    EXPECT_EQ(count, 4);  // Atoms 0, 1, 2, 3
}

TEST_F(SelectionTest, IndexPredicateLessThan) {
    OESelect sel(mol_, "index < 3");

    int count = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol_.GetAtoms(); atom; ++atom) {
        if (sel(*atom)) {
            count++;
        }
    }
    EXPECT_EQ(count, 3);  // Atoms 0, 1, 2
}

TEST_F(SelectionTest, IndexPredicateGreaterThanOrEqual) {
    OESelect sel(mol_, "index >= 10");

    int count = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol_.GetAtoms(); atom; ++atom) {
        if (sel(*atom)) {
            count++;
        }
    }
    int total = static_cast<int>(mol_.NumAtoms());
    EXPECT_EQ(count, total - 10);  // Atoms 10 and higher
}

// Combined tests with logical operators
TEST_F(AtomPropertyTest, ResnAndResiCombined) {
    OESelect sel(mol_, "resn ALA and resi 1");

    int count = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol_.GetAtoms(); atom; ++atom) {
        if (sel(*atom)) {
            count++;
        }
    }
    EXPECT_EQ(count, 5);  // All ALA atoms are in residue 1
}

TEST_F(ElemTest, ElemOrOperator) {
    OESelect sel(mol_, "elem C or elem O");

    int count = 0;
    int expected = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol_.GetAtoms(); atom; ++atom) {
        int anum = atom->GetAtomicNum();
        if (anum == 6 || anum == 8) {
            expected++;
        }
        if (sel(*atom)) {
            count++;
        }
    }
    EXPECT_EQ(count, expected);
}

// ============================================================================
// Tagger Tests (Task 9)
// ============================================================================

TEST(TaggerTest, TagMoleculeWater) {
    OEChem::OEGraphMol mol;
    OEChem::OESmilesToMol(mol, "O");  // Water

    // Set residue name to HOH for all atoms
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
        OEChem::OEResidue res;
        res.SetName("HOH");
        OEChem::OEAtomSetResidue(&(*atom), res);
    }

    Tagger::TagMolecule(mol);
    EXPECT_TRUE(Tagger::IsTagged(mol));

    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
        EXPECT_TRUE(Tagger::HasComponent(*atom, ComponentFlag::Water));
    }
}

TEST(TaggerTest, TagMoleculeWaterVariants) {
    // Test various water residue names
    std::vector<std::string> water_names = {"HOH", "WAT", "H2O", "DOD", "TIP", "TIP3"};

    for (const auto& water_name : water_names) {
        OEChem::OEGraphMol mol;
        OEChem::OESmilesToMol(mol, "O");

        for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
            OEChem::OEResidue res;
            res.SetName(water_name.c_str());
            OEChem::OEAtomSetResidue(&(*atom), res);
        }

        Tagger::TagMolecule(mol);

        for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
            EXPECT_TRUE(Tagger::HasComponent(*atom, ComponentFlag::Water))
                << "Failed for water residue name: " << water_name;
        }
    }
}

TEST(TaggerTest, TagMoleculeProtein) {
    OEChem::OEGraphMol mol;
    // Create a simple alanine-like structure
    OEChem::OESmilesToMol(mol, "CC(N)C(=O)O");

    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
        OEChem::OEResidue res;
        res.SetName("ALA");
        res.SetResidueNumber(1);
        OEChem::OEAtomSetResidue(&(*atom), res);
    }

    Tagger::TagMolecule(mol);
    EXPECT_TRUE(Tagger::IsTagged(mol));

    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
        EXPECT_TRUE(Tagger::HasComponent(*atom, ComponentFlag::Protein));
    }
}

TEST(TaggerTest, TagMoleculeMultipleAminoAcids) {
    // Test multiple amino acid types
    std::vector<std::string> amino_acids = {"ALA", "GLY", "VAL", "LEU", "ILE", "PRO",
                                             "PHE", "TYR", "TRP", "SER", "THR", "CYS",
                                             "MET", "ASN", "GLN", "ASP", "GLU", "LYS",
                                             "ARG", "HIS"};

    for (const auto& aa_name : amino_acids) {
        OEChem::OEGraphMol mol;
        OEChem::OESmilesToMol(mol, "CC(N)C(=O)O");

        for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
            OEChem::OEResidue res;
            res.SetName(aa_name.c_str());
            OEChem::OEAtomSetResidue(&(*atom), res);
        }

        Tagger::TagMolecule(mol);

        for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
            EXPECT_TRUE(Tagger::HasComponent(*atom, ComponentFlag::Protein))
                << "Failed for amino acid: " << aa_name;
        }
    }
}

TEST(TaggerTest, TagMoleculeNucleic) {
    OEChem::OEGraphMol mol;
    OEChem::OESmilesToMol(mol, "c1nc2c(n1)nc[nH]2");  // Purine base

    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
        OEChem::OEResidue res;
        res.SetName("DA");  // Deoxyadenosine
        OEChem::OEAtomSetResidue(&(*atom), res);
    }

    Tagger::TagMolecule(mol);

    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
        EXPECT_TRUE(Tagger::HasComponent(*atom, ComponentFlag::Nucleic));
    }
}

TEST(TaggerTest, TagMoleculeNucleotideVariants) {
    std::vector<std::string> nucleotides = {"A", "G", "C", "T", "U", "DA", "DG", "DC", "DT"};

    for (const auto& nuc_name : nucleotides) {
        OEChem::OEGraphMol mol;
        OEChem::OESmilesToMol(mol, "c1nc2c(n1)nc[nH]2");

        for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
            OEChem::OEResidue res;
            res.SetName(nuc_name.c_str());
            OEChem::OEAtomSetResidue(&(*atom), res);
        }

        Tagger::TagMolecule(mol);

        for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
            EXPECT_TRUE(Tagger::HasComponent(*atom, ComponentFlag::Nucleic))
                << "Failed for nucleotide: " << nuc_name;
        }
    }
}

TEST(TaggerTest, TagMoleculeCofactor) {
    OEChem::OEGraphMol mol;
    OEChem::OESmilesToMol(mol, "Nc1ncnc2c1ncn2C1OC(COP([O-])([O-])=O)C(O)C1O");  // AMP-like

    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
        OEChem::OEResidue res;
        res.SetName("ATP");
        OEChem::OEAtomSetResidue(&(*atom), res);
    }

    Tagger::TagMolecule(mol);

    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
        EXPECT_TRUE(Tagger::HasComponent(*atom, ComponentFlag::Cofactor));
    }
}

TEST(TaggerTest, TagMoleculeCofactorVariants) {
    std::vector<std::string> cofactors = {"NAD", "FAD", "HEM", "ATP", "ADP", "GTP"};

    for (const auto& cof_name : cofactors) {
        OEChem::OEGraphMol mol;
        OEChem::OESmilesToMol(mol, "CC");  // Simple molecule

        for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
            OEChem::OEResidue res;
            res.SetName(cof_name.c_str());
            OEChem::OEAtomSetResidue(&(*atom), res);
        }

        Tagger::TagMolecule(mol);

        for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
            EXPECT_TRUE(Tagger::HasComponent(*atom, ComponentFlag::Cofactor))
                << "Failed for cofactor: " << cof_name;
        }
    }
}

TEST(TaggerTest, TagMoleculeLigand) {
    OEChem::OEGraphMol mol;
    OEChem::OESmilesToMol(mol, "CC(=O)OC1=CC=CC=C1C(=O)O");  // Aspirin

    // Set residue name to something unknown (should default to ligand)
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
        OEChem::OEResidue res;
        res.SetName("LIG");
        OEChem::OEAtomSetResidue(&(*atom), res);
    }

    Tagger::TagMolecule(mol);

    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
        EXPECT_TRUE(Tagger::HasComponent(*atom, ComponentFlag::Ligand));
    }
}

TEST(TaggerTest, TagMoleculeSolvent) {
    OEChem::OEGraphMol mol;
    OEChem::OESmilesToMol(mol, "CS(=O)C");  // DMSO

    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
        OEChem::OEResidue res;
        res.SetName("DMS");  // DMSO residue name
        OEChem::OEAtomSetResidue(&(*atom), res);
    }

    Tagger::TagMolecule(mol);

    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
        EXPECT_TRUE(Tagger::HasComponent(*atom, ComponentFlag::Solvent));
    }
}

TEST(TaggerTest, TagMoleculeIdempotent) {
    OEChem::OEGraphMol mol;
    OEChem::OESmilesToMol(mol, "O");

    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
        OEChem::OEResidue res;
        res.SetName("HOH");
        OEChem::OEAtomSetResidue(&(*atom), res);
    }

    EXPECT_FALSE(Tagger::IsTagged(mol));
    Tagger::TagMolecule(mol);
    EXPECT_TRUE(Tagger::IsTagged(mol));

    // Store original flags
    std::vector<uint32_t> original_flags;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
        original_flags.push_back(Tagger::GetFlags(*atom));
    }

    // Tag again - should be idempotent
    Tagger::TagMolecule(mol);
    EXPECT_TRUE(Tagger::IsTagged(mol));

    // Verify flags are unchanged
    int idx = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
        EXPECT_EQ(Tagger::GetFlags(*atom), original_flags[idx]);
        idx++;
    }
}

TEST(TaggerTest, MixedComponentMolecule) {
    // Create a molecule with mixed components (like a protein-ligand complex)
    OEChem::OEGraphMol mol;
    OEChem::OESmilesToMol(mol, "CC(N)C(=O)O.O.CCO");  // Alanine, water, ethanol

    int atomIdx = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
        OEChem::OEResidue res;
        if (atomIdx < 7) {  // First 7 atoms are alanine
            res.SetName("ALA");
        } else if (atomIdx < 8) {  // Next atom is water
            res.SetName("HOH");
        } else {  // Rest is ethanol (ligand)
            res.SetName("ETH");
        }
        OEChem::OEAtomSetResidue(&(*atom), res);
        atomIdx++;
    }

    Tagger::TagMolecule(mol);
    EXPECT_TRUE(Tagger::IsTagged(mol));

    // Verify each atom has the correct component flag
    atomIdx = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
        if (atomIdx < 7) {
            EXPECT_TRUE(Tagger::HasComponent(*atom, ComponentFlag::Protein))
                << "Atom " << atomIdx << " should be Protein";
        } else if (atomIdx < 8) {
            EXPECT_TRUE(Tagger::HasComponent(*atom, ComponentFlag::Water))
                << "Atom " << atomIdx << " should be Water";
        } else {
            EXPECT_TRUE(Tagger::HasComponent(*atom, ComponentFlag::Ligand))
                << "Atom " << atomIdx << " should be Ligand";
        }
        atomIdx++;
    }
}

TEST(TaggerTest, GetFlagsReturnsZeroForUntaggedAtom) {
    OEChem::OEGraphMol mol;
    OEChem::OESmilesToMol(mol, "C");

    // Don't tag the molecule
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
        EXPECT_EQ(Tagger::GetFlags(*atom), 0u);
    }
}

TEST(TaggerTest, HasComponentReturnsFalseForUntaggedAtom) {
    OEChem::OEGraphMol mol;
    OEChem::OESmilesToMol(mol, "C");

    // Don't tag the molecule
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
        EXPECT_FALSE(Tagger::HasComponent(*atom, ComponentFlag::Protein));
        EXPECT_FALSE(Tagger::HasComponent(*atom, ComponentFlag::Water));
        EXPECT_FALSE(Tagger::HasComponent(*atom, ComponentFlag::Ligand));
    }
}

TEST(TaggerTest, ProtonationStateVariants) {
    // Test common protonation state variants of amino acids
    std::vector<std::string> protonation_variants = {"HID", "HIE", "HIP", "CYX", "ASH", "GLH"};

    for (const auto& res_name : protonation_variants) {
        OEChem::OEGraphMol mol;
        OEChem::OESmilesToMol(mol, "CC");

        for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
            OEChem::OEResidue res;
            res.SetName(res_name.c_str());
            OEChem::OEAtomSetResidue(&(*atom), res);
        }

        Tagger::TagMolecule(mol);

        for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
            EXPECT_TRUE(Tagger::HasComponent(*atom, ComponentFlag::Protein))
                << "Failed for protonation variant: " << res_name;
        }
    }
}

// ============================================================================
// Component Predicate Tests (Task 10)
// ============================================================================

TEST(ComponentPredicateTest, ProteinPredicate) {
    OEChem::OEGraphMol mol;
    OEChem::OESmilesToMol(mol, "CC(N)C(=O)O");  // Alanine

    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
        OEChem::OEResidue res;
        res.SetName("ALA");
        res.SetResidueNumber(1);
        OEChem::OEAtomSetResidue(&(*atom), res);
    }

    OESelect sel(mol, "protein");
    int count = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
        if (sel(*atom)) count++;
    }
    EXPECT_EQ(count, static_cast<int>(mol.NumAtoms()));
}

TEST(ComponentPredicateTest, ProteinPredicateMultipleResidues) {
    OEChem::OEGraphMol mol;
    OEChem::OESmilesToMol(mol, "CC(N)C(=O)NCC(=O)O");  // Ala-Gly dipeptide

    std::vector<std::string> residues = {"ALA", "GLY", "VAL", "LEU", "ILE"};
    int atomIdx = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
        OEChem::OEResidue res;
        res.SetName(residues[atomIdx % residues.size()].c_str());
        OEChem::OEAtomSetResidue(&(*atom), res);
        atomIdx++;
    }

    OESelect sel(mol, "protein");
    int count = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
        if (sel(*atom)) count++;
    }
    EXPECT_EQ(count, static_cast<int>(mol.NumAtoms()));
}

TEST(ComponentPredicateTest, LigandPredicate) {
    OEChem::OEGraphMol mol;
    OEChem::OESmilesToMol(mol, "CC(=O)OC1=CC=CC=C1C(=O)O");  // Aspirin

    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
        OEChem::OEResidue res;
        res.SetName("LIG");  // Unknown residue name -> ligand
        OEChem::OEAtomSetResidue(&(*atom), res);
    }

    OESelect sel(mol, "ligand");
    int count = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
        if (sel(*atom)) count++;
    }
    EXPECT_EQ(count, static_cast<int>(mol.NumAtoms()));
}

TEST(ComponentPredicateTest, WaterPredicate) {
    OEChem::OEGraphMol mol;
    OEChem::OESmilesToMol(mol, "O");

    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
        OEChem::OEResidue res;
        res.SetName("HOH");
        OEChem::OEAtomSetResidue(&(*atom), res);
    }

    OESelect sel(mol, "water");
    int count = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
        if (sel(*atom)) count++;
    }
    EXPECT_EQ(count, static_cast<int>(mol.NumAtoms()));
}

TEST(ComponentPredicateTest, WaterPredicateVariants) {
    std::vector<std::string> water_names = {"HOH", "WAT", "H2O", "DOD", "TIP"};

    for (const auto& water_name : water_names) {
        OEChem::OEGraphMol mol;
        OEChem::OESmilesToMol(mol, "O");

        for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
            OEChem::OEResidue res;
            res.SetName(water_name.c_str());
            OEChem::OEAtomSetResidue(&(*atom), res);
        }

        OESelect sel(mol, "water");
        int count = 0;
        for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
            if (sel(*atom)) count++;
        }
        EXPECT_EQ(count, static_cast<int>(mol.NumAtoms()))
            << "Failed for water name: " << water_name;
    }
}

TEST(ComponentPredicateTest, SolventPredicate) {
    OEChem::OEGraphMol mol;
    OEChem::OESmilesToMol(mol, "CS(=O)C");  // DMSO

    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
        OEChem::OEResidue res;
        res.SetName("DMS");  // DMSO residue name
        OEChem::OEAtomSetResidue(&(*atom), res);
    }

    OESelect sel(mol, "solvent");
    int count = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
        if (sel(*atom)) count++;
    }
    EXPECT_EQ(count, static_cast<int>(mol.NumAtoms()));
}

TEST(ComponentPredicateTest, SolventPredicateIncludesWater) {
    OEChem::OEGraphMol mol;
    OEChem::OESmilesToMol(mol, "O");

    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
        OEChem::OEResidue res;
        res.SetName("HOH");
        OEChem::OEAtomSetResidue(&(*atom), res);
    }

    // Solvent should include water
    OESelect sel(mol, "solvent");
    int count = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
        if (sel(*atom)) count++;
    }
    EXPECT_EQ(count, static_cast<int>(mol.NumAtoms()));
}

TEST(ComponentPredicateTest, OrganicPredicate) {
    OEChem::OEGraphMol mol;
    OEChem::OESmilesToMol(mol, "CC(=O)OC1=CC=CC=C1C(=O)O");  // Aspirin

    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
        OEChem::OEResidue res;
        res.SetName("LIG");
        OEChem::OEAtomSetResidue(&(*atom), res);
    }

    OESelect sel(mol, "organic");
    int count = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
        if (sel(*atom)) count++;
    }
    // All atoms in aspirin should be organic (C-containing)
    EXPECT_EQ(count, static_cast<int>(mol.NumAtoms()));
}

TEST(ComponentPredicateTest, OrganicPredicateExcludesProtein) {
    OEChem::OEGraphMol mol;
    OEChem::OESmilesToMol(mol, "CC(N)C(=O)O");  // Alanine

    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
        OEChem::OEResidue res;
        res.SetName("ALA");
        OEChem::OEAtomSetResidue(&(*atom), res);
    }

    OESelect sel(mol, "organic");
    int count = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
        if (sel(*atom)) count++;
    }
    // Protein atoms should not be organic
    EXPECT_EQ(count, 0);
}

TEST(ComponentPredicateTest, BackbonePredicate) {
    OEChem::OEGraphMol mol;
    OEChem::OESmilesToMol(mol, "CC(N)C(=O)O");  // Alanine

    // Set up proper backbone atom names
    std::vector<std::string> names = {"CB", "CA", "N", "C", "O", "OXT", "HN"};
    int idx = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
        OEChem::OEResidue res;
        res.SetName("ALA");
        res.SetResidueNumber(1);
        OEChem::OEAtomSetResidue(&(*atom), res);
        if (idx < static_cast<int>(names.size())) {
            atom->SetName(names[idx].c_str());
        }
        idx++;
    }

    OESelect sel(mol, "backbone");
    int count = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
        if (sel(*atom)) {
            count++;
            std::string name = atom->GetName();
            EXPECT_TRUE(name == "N" || name == "CA" || name == "C" || name == "O")
                << "Unexpected backbone atom: " << name;
        }
    }
    // Should match N, CA, C, O (4 backbone atoms)
    EXPECT_EQ(count, 4);
}

TEST(ComponentPredicateTest, BackbonePredicateAlias) {
    OEChem::OEGraphMol mol;
    OEChem::OESmilesToMol(mol, "CC(N)C(=O)O");

    std::vector<std::string> names = {"CB", "CA", "N", "C", "O"};
    int idx = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
        OEChem::OEResidue res;
        res.SetName("ALA");
        OEChem::OEAtomSetResidue(&(*atom), res);
        if (idx < static_cast<int>(names.size())) {
            atom->SetName(names[idx].c_str());
        }
        idx++;
    }

    // Test "bb" alias
    OESelect sel_bb(mol, "bb");
    OESelect sel_backbone(mol, "backbone");

    int count_bb = 0, count_backbone = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
        if (sel_bb(*atom)) count_bb++;
        if (sel_backbone(*atom)) count_backbone++;
    }
    EXPECT_EQ(count_bb, count_backbone);
}

TEST(ComponentPredicateTest, SidechainPredicate) {
    OEChem::OEGraphMol mol;
    OEChem::OESmilesToMol(mol, "CC(N)C(=O)O");  // Alanine - 6 heavy atoms

    // Assign atom names to match alanine: CB, CA, N, C, O, OXT
    std::vector<std::string> names = {"CB", "CA", "N", "C", "O", "OXT"};
    int idx = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
        OEChem::OEResidue res;
        res.SetName("ALA");
        res.SetResidueNumber(1);
        OEChem::OEAtomSetResidue(&(*atom), res);
        if (idx < static_cast<int>(names.size())) {
            atom->SetName(names[idx].c_str());
        }
        idx++;
    }

    OESelect sel(mol, "sidechain");
    int count = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
        if (sel(*atom)) {
            count++;
            std::string name = atom->GetName();
            // Sidechain should NOT include N, CA, C, O, or OXT
            EXPECT_TRUE(name != "N" && name != "CA" && name != "C" && name != "O" && name != "OXT")
                << "Unexpected sidechain atom: " << name;
        }
    }
    // Should match CB (1 sidechain atom)
    EXPECT_EQ(count, 1);
}

TEST(ComponentPredicateTest, SidechainPredicateAlias) {
    OEChem::OEGraphMol mol;
    OEChem::OESmilesToMol(mol, "CC(N)C(=O)O");

    std::vector<std::string> names = {"CB", "CA", "N", "C", "O"};
    int idx = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
        OEChem::OEResidue res;
        res.SetName("ALA");
        OEChem::OEAtomSetResidue(&(*atom), res);
        if (idx < static_cast<int>(names.size())) {
            atom->SetName(names[idx].c_str());
        }
        idx++;
    }

    // Test "sc" alias
    OESelect sel_sc(mol, "sc");
    OESelect sel_sidechain(mol, "sidechain");

    int count_sc = 0, count_sidechain = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
        if (sel_sc(*atom)) count_sc++;
        if (sel_sidechain(*atom)) count_sidechain++;
    }
    EXPECT_EQ(count_sc, count_sidechain);
}

TEST(ComponentPredicateTest, MetalPredicate) {
    OEChem::OEGraphMol mol;
    mol.NewAtom(26);  // Fe
    mol.NewAtom(30);  // Zn
    mol.NewAtom(6);   // C (not a metal)

    OESelect sel(mol, "metal");
    int count = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
        if (sel(*atom)) count++;
    }
    EXPECT_EQ(count, 2);  // Fe and Zn
}

TEST(ComponentPredicateTest, MetalPredicateAlias) {
    OEChem::OEGraphMol mol;
    mol.NewAtom(26);  // Fe
    mol.NewAtom(30);  // Zn

    // Test "metals" alias
    OESelect sel_metals(mol, "metals");
    OESelect sel_metal(mol, "metal");

    int count_metals = 0, count_metal = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
        if (sel_metals(*atom)) count_metals++;
        if (sel_metal(*atom)) count_metal++;
    }
    EXPECT_EQ(count_metals, count_metal);
    EXPECT_EQ(count_metals, 2);
}

TEST(ComponentPredicateTest, MetalPredicateCommonMetals) {
    // Test various metal elements
    std::vector<int> metals = {
        3,   // Li
        11,  // Na
        12,  // Mg
        19,  // K
        20,  // Ca
        25,  // Mn
        26,  // Fe
        27,  // Co
        28,  // Ni
        29,  // Cu
        30,  // Zn
        42   // Mo
    };

    OEChem::OEGraphMol mol;
    for (int z : metals) {
        mol.NewAtom(z);
    }

    OESelect sel(mol, "metal");
    int count = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
        if (sel(*atom)) count++;
    }
    EXPECT_EQ(count, static_cast<int>(metals.size()));
}

TEST(ComponentPredicateTest, NotProtein) {
    OEChem::OEGraphMol mol;
    OEChem::OESmilesToMol(mol, "CC(N)C(=O)O.O");  // Alanine (6 atoms) + water (1 atom)

    int atomIdx = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
        OEChem::OEResidue res;
        if (atomIdx < 6) {  // First 6 atoms are alanine
            res.SetName("ALA");
        } else {  // Water
            res.SetName("HOH");
        }
        OEChem::OEAtomSetResidue(&(*atom), res);
        atomIdx++;
    }

    OESelect sel(mol, "not protein");
    int count = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
        if (sel(*atom)) count++;
    }
    // Only water atom should match "not protein"
    EXPECT_EQ(count, 1);
}

TEST(ComponentPredicateTest, ProteinAndBackbone) {
    OEChem::OEGraphMol mol;
    OEChem::OESmilesToMol(mol, "CC(N)C(=O)O");  // Alanine (6 heavy atoms)

    // Assign names: CB, CA, N, C, O, OXT to all 6 atoms
    std::vector<std::string> names = {"CB", "CA", "N", "C", "O", "OXT"};
    int idx = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
        OEChem::OEResidue res;
        res.SetName("ALA");
        OEChem::OEAtomSetResidue(&(*atom), res);
        if (idx < static_cast<int>(names.size())) {
            atom->SetName(names[idx].c_str());
        }
        idx++;
    }

    OESelect sel(mol, "protein and backbone");
    int count = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
        if (sel(*atom)) count++;
    }
    // Should match N, CA, C, O (4 backbone atoms in protein - OXT and CB are not backbone)
    EXPECT_EQ(count, 4);
}

TEST(ComponentPredicateTest, ProteinOrWater) {
    OEChem::OEGraphMol mol;
    // Alanine: CC(N)C(=O)O = 6 atoms
    // Water: O = 1 atom
    // Ethanol: CCO = 3 atoms
    OEChem::OESmilesToMol(mol, "CC(N)C(=O)O.O.CCO");

    int atomIdx = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
        OEChem::OEResidue res;
        if (atomIdx < 6) {  // Alanine (6 atoms)
            res.SetName("ALA");
        } else if (atomIdx < 7) {  // Water (1 atom)
            res.SetName("HOH");
        } else {  // Ethanol (3 atoms - ligand)
            res.SetName("ETH");
        }
        OEChem::OEAtomSetResidue(&(*atom), res);
        atomIdx++;
    }

    OESelect sel(mol, "protein or water");
    int protein_and_water_count = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
        if (sel(*atom)) protein_and_water_count++;
    }

    // Should not include ethanol atoms
    EXPECT_LT(protein_and_water_count, static_cast<int>(mol.NumAtoms()));
    // Should include all alanine and water atoms
    EXPECT_EQ(protein_and_water_count, 7);  // 6 alanine + 1 water
}

TEST(ComponentPredicateTest, CaseInsensitive) {
    OEChem::OEGraphMol mol;
    OEChem::OESmilesToMol(mol, "CC(N)C(=O)O");

    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
        OEChem::OEResidue res;
        res.SetName("ALA");
        OEChem::OEAtomSetResidue(&(*atom), res);
    }

    OESelect sel_lower(mol, "protein");
    OESelect sel_upper(mol, "PROTEIN");
    OESelect sel_mixed(mol, "Protein");

    int count_lower = 0, count_upper = 0, count_mixed = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
        if (sel_lower(*atom)) count_lower++;
        if (sel_upper(*atom)) count_upper++;
        if (sel_mixed(*atom)) count_mixed++;
    }
    EXPECT_EQ(count_lower, count_upper);
    EXPECT_EQ(count_lower, count_mixed);
}

// ============================================================================
// Atom Type Predicate Tests (Task 11)
// ============================================================================

TEST(AtomTypePredicateTest, HeavyPredicate) {
    OEChem::OEGraphMol mol;
    OEChem::OESmilesToMol(mol, "C");  // Methane (1 C + 4 H)
    OEChem::OEAddExplicitHydrogens(mol);

    OESelect sel(mol, "heavy");
    int count = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
        if (sel(*atom)) count++;
    }
    EXPECT_EQ(count, 1);  // Only the carbon
}

TEST(AtomTypePredicateTest, HydrogenPredicate) {
    OEChem::OEGraphMol mol;
    OEChem::OESmilesToMol(mol, "C");
    OEChem::OEAddExplicitHydrogens(mol);

    OESelect sel(mol, "hydrogen");
    int count = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
        if (sel(*atom)) count++;
    }
    EXPECT_EQ(count, 4);  // 4 hydrogens
}

TEST(AtomTypePredicateTest, HydrogenAlias) {
    OEChem::OEGraphMol mol;
    OEChem::OESmilesToMol(mol, "C");
    OEChem::OEAddExplicitHydrogens(mol);

    OESelect sel(mol, "h");  // Short alias
    int count = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
        if (sel(*atom)) count++;
    }
    EXPECT_EQ(count, 4);
}

TEST(AtomTypePredicateTest, PolarHydrogenPredicate) {
    OEChem::OEGraphMol mol;
    OEChem::OESmilesToMol(mol, "O");  // Water
    OEChem::OEAddExplicitHydrogens(mol);

    OESelect sel(mol, "polar_hydrogen");
    int count = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
        if (sel(*atom)) count++;
    }
    EXPECT_EQ(count, 2);  // Both H's bonded to O
}

TEST(AtomTypePredicateTest, PolarHydrogenAlias) {
    OEChem::OEGraphMol mol;
    OEChem::OESmilesToMol(mol, "O");  // Water
    OEChem::OEAddExplicitHydrogens(mol);

    OESelect sel(mol, "polarh");  // Short alias
    int count = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
        if (sel(*atom)) count++;
    }
    EXPECT_EQ(count, 2);
}

TEST(AtomTypePredicateTest, NonpolarHydrogenPredicate) {
    OEChem::OEGraphMol mol;
    OEChem::OESmilesToMol(mol, "C");  // Methane
    OEChem::OEAddExplicitHydrogens(mol);

    OESelect sel(mol, "nonpolar_hydrogen");
    int count = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
        if (sel(*atom)) count++;
    }
    EXPECT_EQ(count, 4);  // All 4 H's bonded to C
}

TEST(AtomTypePredicateTest, NonpolarHydrogenAlias) {
    OEChem::OEGraphMol mol;
    OEChem::OESmilesToMol(mol, "C");  // Methane
    OEChem::OEAddExplicitHydrogens(mol);

    OESelect sel(mol, "apolarh");  // Short alias
    int count = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
        if (sel(*atom)) count++;
    }
    EXPECT_EQ(count, 4);
}

TEST(AtomTypePredicateTest, MixedPolarNonpolar) {
    OEChem::OEGraphMol mol;
    OEChem::OESmilesToMol(mol, "CO");  // Methanol: CH3-OH
    OEChem::OEAddExplicitHydrogens(mol);

    OESelect polar(mol, "polarh");
    OESelect nonpolar(mol, "apolarh");

    int polar_count = 0, nonpolar_count = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
        if (polar(*atom)) polar_count++;
        if (nonpolar(*atom)) nonpolar_count++;
    }
    EXPECT_EQ(polar_count, 1);     // 1 H on O
    EXPECT_EQ(nonpolar_count, 3);  // 3 H's on C
}

TEST(AtomTypePredicateTest, HeavyAndNotHydrogen) {
    // Verify heavy == not hydrogen
    OEChem::OEGraphMol mol;
    OEChem::OESmilesToMol(mol, "CCO");
    OEChem::OEAddExplicitHydrogens(mol);

    OESelect heavy(mol, "heavy");
    OESelect notH(mol, "not hydrogen");

    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
        EXPECT_EQ(heavy(*atom), notH(*atom));
    }
}

TEST(AtomTypePredicateTest, PolarHydrogenNitrogen) {
    // Test hydrogen bonded to nitrogen
    OEChem::OEGraphMol mol;
    OEChem::OESmilesToMol(mol, "N");  // Ammonia
    OEChem::OEAddExplicitHydrogens(mol);

    OESelect sel(mol, "polar_hydrogen");
    int count = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
        if (sel(*atom)) count++;
    }
    EXPECT_EQ(count, 3);  // 3 H's bonded to N
}

TEST(AtomTypePredicateTest, PolarHydrogenSulfur) {
    // Test hydrogen bonded to sulfur (thiol)
    OEChem::OEGraphMol mol;
    OEChem::OESmilesToMol(mol, "CS");  // Methanethiol
    OEChem::OEAddExplicitHydrogens(mol);

    OESelect sel(mol, "polar_hydrogen");
    int count = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
        if (sel(*atom)) count++;
    }
    EXPECT_EQ(count, 1);  // 1 H on S (the SH hydrogen)
}

TEST(AtomTypePredicateTest, HeavyAtomCount) {
    OEChem::OEGraphMol mol;
    OEChem::OESmilesToMol(mol, "CCO");  // Ethanol
    OEChem::OEAddExplicitHydrogens(mol);

    OESelect sel(mol, "heavy");
    int count = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
        if (sel(*atom)) count++;
    }
    // Ethanol has 3 heavy atoms: C, C, O
    EXPECT_EQ(count, 3);
}

TEST(AtomTypePredicateTest, HydrogenAndHeavyMutuallyExclusive) {
    // Verify hydrogen and heavy are mutually exclusive
    OEChem::OEGraphMol mol;
    OEChem::OESmilesToMol(mol, "CCO");
    OEChem::OEAddExplicitHydrogens(mol);

    OESelect hydrogen(mol, "hydrogen");
    OESelect heavy(mol, "heavy");

    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
        // An atom cannot be both hydrogen AND heavy
        EXPECT_FALSE(hydrogen(*atom) && heavy(*atom));
        // An atom must be either hydrogen OR heavy
        EXPECT_TRUE(hydrogen(*atom) || heavy(*atom));
    }
}

TEST(AtomTypePredicateTest, PolarAndNonpolarMutuallyExclusive) {
    // Verify polar and nonpolar hydrogens are mutually exclusive
    OEChem::OEGraphMol mol;
    OEChem::OESmilesToMol(mol, "CO");  // Methanol
    OEChem::OEAddExplicitHydrogens(mol);

    OESelect polarh(mol, "polarh");
    OESelect apolarh(mol, "apolarh");

    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
        // A hydrogen cannot be both polar AND nonpolar
        EXPECT_FALSE(polarh(*atom) && apolarh(*atom));
    }
}

TEST(AtomTypePredicateTest, CombinedWithLogicalOperators) {
    OEChem::OEGraphMol mol;
    OEChem::OESmilesToMol(mol, "CO");  // Methanol
    OEChem::OEAddExplicitHydrogens(mol);

    // Select heavy atoms or polar hydrogens
    OESelect sel(mol, "heavy or polarh");
    int count = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol.GetAtoms(); atom; ++atom) {
        if (sel(*atom)) count++;
    }
    // 2 heavy atoms (C, O) + 1 polar hydrogen = 3
    EXPECT_EQ(count, 3);
}

// ============================================================================
// Distance Predicate Tests (Task 13)
// ============================================================================

class DistancePredicateTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create atoms at known positions for distance testing
        // REF at origin (0, 0, 0)
        // NEAR at (1.5, 0, 0) - 1.5 angstroms from origin
        // MID at (4.0, 0, 0) - 4.0 angstroms from origin
        // FAR at (10.0, 0, 0) - 10.0 angstroms from origin

        mol_.NewAtom(6);  // Atom 0 - REF
        mol_.NewAtom(6);  // Atom 1 - NEAR
        mol_.NewAtom(6);  // Atom 2 - MID
        mol_.NewAtom(6);  // Atom 3 - FAR

        float coords[4][3] = {
            {0.0f, 0.0f, 0.0f},     // REF
            {1.5f, 0.0f, 0.0f},     // NEAR
            {4.0f, 0.0f, 0.0f},     // MID
            {10.0f, 0.0f, 0.0f}     // FAR
        };

        int idx = 0;
        for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol_.GetAtoms(); atom; ++atom) {
            mol_.SetCoords(&(*atom), coords[idx]);
            switch (idx) {
                case 0: atom->SetName("REF"); break;
                case 1: atom->SetName("NEAR"); break;
                case 2: atom->SetName("MID"); break;
                case 3: atom->SetName("FAR"); break;
            }
            idx++;
        }
    }

    OEChem::OEGraphMol mol_;
};

TEST_F(DistancePredicateTest, AroundBasic) {
    // around 3.0 name REF should match REF (at 0) and NEAR (at 1.5)
    OESelect sel(mol_, "around 3.0 name REF");

    int count = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol_.GetAtoms(); atom; ++atom) {
        std::string name = atom->GetName();
        bool matches = sel(*atom);
        if (name == "REF") {
            EXPECT_TRUE(matches) << "REF should be within around selection (distance 0)";
        } else if (name == "NEAR") {
            EXPECT_TRUE(matches) << "NEAR should be within around selection (distance 1.5)";
        } else if (name == "MID") {
            EXPECT_FALSE(matches) << "MID should NOT be within around selection (distance 4.0)";
        } else if (name == "FAR") {
            EXPECT_FALSE(matches) << "FAR should NOT be within around selection (distance 10.0)";
        }
        if (matches) count++;
    }
    EXPECT_EQ(count, 2);  // REF and NEAR
}

TEST_F(DistancePredicateTest, AroundLargerRadius) {
    // around 5.0 name REF should match REF, NEAR, and MID
    OESelect sel(mol_, "around 5.0 name REF");

    int count = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol_.GetAtoms(); atom; ++atom) {
        if (sel(*atom)) count++;
    }
    EXPECT_EQ(count, 3);  // REF, NEAR, MID
}

TEST_F(DistancePredicateTest, XAroundExcludesReference) {
    // xaround 3.0 name REF should match only NEAR (excludes REF itself)
    OESelect sel(mol_, "xaround 3.0 name REF");

    int count = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol_.GetAtoms(); atom; ++atom) {
        std::string name = atom->GetName();
        bool matches = sel(*atom);
        if (name == "REF") {
            EXPECT_FALSE(matches) << "REF should be EXCLUDED from xaround selection";
        } else if (name == "NEAR") {
            EXPECT_TRUE(matches) << "NEAR should be within xaround selection";
        } else if (name == "MID") {
            EXPECT_FALSE(matches) << "MID should NOT be within xaround selection";
        } else if (name == "FAR") {
            EXPECT_FALSE(matches) << "FAR should NOT be within xaround selection";
        }
        if (matches) count++;
    }
    EXPECT_EQ(count, 1);  // Only NEAR
}

TEST_F(DistancePredicateTest, XAroundLargerRadius) {
    // xaround 5.0 name REF should match NEAR and MID (excludes REF)
    OESelect sel(mol_, "xaround 5.0 name REF");

    int count = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol_.GetAtoms(); atom; ++atom) {
        if (sel(*atom)) count++;
    }
    EXPECT_EQ(count, 2);  // NEAR and MID
}

TEST_F(DistancePredicateTest, BeyondBasic) {
    // beyond 3.0 name REF should match MID and FAR
    OESelect sel(mol_, "beyond 3.0 name REF");

    int count = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol_.GetAtoms(); atom; ++atom) {
        std::string name = atom->GetName();
        bool matches = sel(*atom);
        if (name == "REF") {
            EXPECT_FALSE(matches) << "REF should NOT be beyond selection (distance 0)";
        } else if (name == "NEAR") {
            EXPECT_FALSE(matches) << "NEAR should NOT be beyond selection (distance 1.5)";
        } else if (name == "MID") {
            EXPECT_TRUE(matches) << "MID should be beyond selection (distance 4.0)";
        } else if (name == "FAR") {
            EXPECT_TRUE(matches) << "FAR should be beyond selection (distance 10.0)";
        }
        if (matches) count++;
    }
    EXPECT_EQ(count, 2);  // MID and FAR
}

TEST_F(DistancePredicateTest, BeyondLargerRadius) {
    // beyond 5.0 name REF should match only FAR
    OESelect sel(mol_, "beyond 5.0 name REF");

    int count = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol_.GetAtoms(); atom; ++atom) {
        if (sel(*atom)) count++;
    }
    EXPECT_EQ(count, 1);  // Only FAR
}

TEST_F(DistancePredicateTest, AroundWithParentheses) {
    // Test around with parenthesized selection
    OESelect sel(mol_, "around 3.0 (name REF or name NEAR)");

    int count = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol_.GetAtoms(); atom; ++atom) {
        if (sel(*atom)) count++;
    }
    // REF and NEAR are reference atoms
    // Atoms within 3.0 of REF: REF, NEAR
    // Atoms within 3.0 of NEAR: REF, NEAR, MID (MID is 2.5 from NEAR)
    // Union: REF, NEAR, MID
    EXPECT_EQ(count, 3);
}

TEST_F(DistancePredicateTest, AroundFloatingPoint) {
    // Test floating point radius parsing
    OESelect sel(mol_, "around 1.6 name REF");

    int count = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol_.GetAtoms(); atom; ++atom) {
        if (sel(*atom)) count++;
    }
    // REF (0) and NEAR (1.5) are within 1.6
    EXPECT_EQ(count, 2);
}

TEST_F(DistancePredicateTest, AroundExactBoundary) {
    // Test boundary case - radius exactly matches distance
    OESelect sel(mol_, "around 1.5 name REF");

    int count = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol_.GetAtoms(); atom; ++atom) {
        if (sel(*atom)) count++;
    }
    // REF (0) is within, NEAR (1.5) is on boundary
    // nanoflann uses <= for radius search
    EXPECT_GE(count, 1);  // At least REF
}

TEST_F(DistancePredicateTest, AroundCombinedWithAnd) {
    // Test distance predicate combined with logical operator
    OESelect sel(mol_, "around 5.0 name REF and not name REF");

    int count = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol_.GetAtoms(); atom; ++atom) {
        if (sel(*atom)) count++;
    }
    // Same as xaround - within 5.0 but not REF itself
    EXPECT_EQ(count, 2);  // NEAR and MID
}

TEST_F(DistancePredicateTest, BeyondAll) {
    // Test beyond with large radius - should match nothing
    OESelect sel(mol_, "beyond 100.0 name REF");

    int count = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol_.GetAtoms(); atom; ++atom) {
        if (sel(*atom)) count++;
    }
    EXPECT_EQ(count, 0);  // All atoms are within 100.0 of REF
}

TEST_F(DistancePredicateTest, AroundMultipleReferenceAtoms) {
    // Create a molecule with multiple reference atoms in different positions
    OEChem::OEGraphMol mol2;
    mol2.NewAtom(6);  // REF1 at origin
    mol2.NewAtom(6);  // REF2 at (5, 0, 0)
    mol2.NewAtom(6);  // TARGET at (2.5, 0, 0) - equidistant
    mol2.NewAtom(6);  // FAR at (10, 0, 0)

    float coords[4][3] = {
        {0.0f, 0.0f, 0.0f},
        {5.0f, 0.0f, 0.0f},
        {2.5f, 0.0f, 0.0f},
        {10.0f, 0.0f, 0.0f}
    };

    int idx = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol2.GetAtoms(); atom; ++atom) {
        mol2.SetCoords(&(*atom), coords[idx]);
        switch (idx) {
            case 0: atom->SetName("REF1"); break;
            case 1: atom->SetName("REF2"); break;
            case 2: atom->SetName("TARGET"); break;
            case 3: atom->SetName("FAR"); break;
        }
        idx++;
    }

    // around 3.0 with REF1 or REF2 as reference
    OESelect sel(mol2, "around 3.0 (name REF1 or name REF2)");

    int count = 0;
    for (OESystem::OEIter<OEChem::OEAtomBase> atom = mol2.GetAtoms(); atom; ++atom) {
        if (sel(*atom)) count++;
    }
    // REF1 (within 0 of itself), REF2 (within 0 of itself), TARGET (2.5 from both)
    // FAR is 10 from REF1 and 5 from REF2 - too far from both
    EXPECT_EQ(count, 3);  // REF1, REF2, TARGET
}

TEST_F(DistancePredicateTest, ToCanonicalAround) {
    OESelection sele = OESelection::Parse("around 5.0 name REF");
    std::string canonical = sele.ToCanonical();
    EXPECT_TRUE(canonical.find("around") != std::string::npos);
    EXPECT_TRUE(canonical.find("5") != std::string::npos);
}

TEST_F(DistancePredicateTest, ToCanonicalXAround) {
    OESelection sele = OESelection::Parse("xaround 3.5 name CA");
    std::string canonical = sele.ToCanonical();
    EXPECT_TRUE(canonical.find("xaround") != std::string::npos);
    EXPECT_TRUE(canonical.find("3.5") != std::string::npos);
}

TEST_F(DistancePredicateTest, ToCanonicalBeyond) {
    OESelection sele = OESelection::Parse("beyond 10.0 water");
    std::string canonical = sele.ToCanonical();
    EXPECT_TRUE(canonical.find("beyond") != std::string::npos);
    EXPECT_TRUE(canonical.find("10") != std::string::npos);
}

// ============================================================================
// Expansion Predicate Tests (Task 14)
// ============================================================================

TEST(ExpansionPredicateTest, ByResBasic) {
    OEChem::OEGraphMol mol;
    // Create atoms with residue info
    auto* a1 = mol.NewAtom(6);
    auto* a2 = mol.NewAtom(7);
    auto* a3 = mol.NewAtom(8);

    a1->SetName("CA");
    a2->SetName("N");
    a3->SetName("O");

    // Put a1 and a2 in residue 1, a3 in residue 2
    OEChem::OEResidue res1, res2;
    res1.SetName("ALA");
    res1.SetResidueNumber(1);
    res1.SetChainID('A');
    res2.SetName("GLY");
    res2.SetResidueNumber(2);
    res2.SetChainID('A');

    OEChem::OEAtomSetResidue(a1, res1);
    OEChem::OEAtomSetResidue(a2, res1);
    OEChem::OEAtomSetResidue(a3, res2);

    // byres name CA should select a1 AND a2 (both in residue 1)
    OESelect sel(mol, "byres name CA");

    EXPECT_TRUE(sel(*a1));   // CA itself
    EXPECT_TRUE(sel(*a2));   // N in same residue
    EXPECT_FALSE(sel(*a3));  // O in different residue
}

TEST(ExpansionPredicateTest, ByResMultipleMatchingAtoms) {
    OEChem::OEGraphMol mol;
    // Create atoms in 3 residues
    auto* a1 = mol.NewAtom(6);  // Res 1
    auto* a2 = mol.NewAtom(7);  // Res 1
    auto* a3 = mol.NewAtom(6);  // Res 2 - another CA
    auto* a4 = mol.NewAtom(8);  // Res 2
    auto* a5 = mol.NewAtom(7);  // Res 3 - no CA

    a1->SetName("CA");
    a2->SetName("N");
    a3->SetName("CA");
    a4->SetName("O");
    a5->SetName("N");

    OEChem::OEResidue res1, res2, res3;
    res1.SetName("ALA");
    res1.SetResidueNumber(1);
    res1.SetChainID('A');
    res2.SetName("GLY");
    res2.SetResidueNumber(2);
    res2.SetChainID('A');
    res3.SetName("VAL");
    res3.SetResidueNumber(3);
    res3.SetChainID('A');

    OEChem::OEAtomSetResidue(a1, res1);
    OEChem::OEAtomSetResidue(a2, res1);
    OEChem::OEAtomSetResidue(a3, res2);
    OEChem::OEAtomSetResidue(a4, res2);
    OEChem::OEAtomSetResidue(a5, res3);

    // byres name CA should select all atoms in residues containing CA
    OESelect sel(mol, "byres name CA");

    EXPECT_TRUE(sel(*a1));   // CA in res 1
    EXPECT_TRUE(sel(*a2));   // N in res 1 (has CA)
    EXPECT_TRUE(sel(*a3));   // CA in res 2
    EXPECT_TRUE(sel(*a4));   // O in res 2 (has CA)
    EXPECT_FALSE(sel(*a5));  // N in res 3 (no CA)
}

TEST(ExpansionPredicateTest, ByChainBasic) {
    OEChem::OEGraphMol mol;
    auto* a1 = mol.NewAtom(6);
    auto* a2 = mol.NewAtom(7);
    auto* a3 = mol.NewAtom(8);

    a1->SetName("CA");
    a2->SetName("N");
    a3->SetName("O");

    OEChem::OEResidue resA1, resA2, resB;
    resA1.SetChainID('A');
    resA1.SetResidueNumber(1);
    resA1.SetName("ALA");
    resA2.SetChainID('A');
    resA2.SetResidueNumber(2);
    resA2.SetName("GLY");
    resB.SetChainID('B');
    resB.SetResidueNumber(1);
    resB.SetName("VAL");

    OEChem::OEAtomSetResidue(a1, resA1);
    OEChem::OEAtomSetResidue(a2, resA2);  // Same chain A, different residue
    OEChem::OEAtomSetResidue(a3, resB);   // Chain B

    // bychain name CA should select a1 and a2 (both in chain A)
    OESelect sel(mol, "bychain name CA");

    EXPECT_TRUE(sel(*a1));   // CA in chain A
    EXPECT_TRUE(sel(*a2));   // N in chain A (same chain as CA)
    EXPECT_FALSE(sel(*a3));  // O in chain B
}

TEST(ExpansionPredicateTest, ByChainMultipleChains) {
    OEChem::OEGraphMol mol;
    auto* a1 = mol.NewAtom(6);  // Chain A - has CA
    auto* a2 = mol.NewAtom(7);  // Chain A
    auto* a3 = mol.NewAtom(6);  // Chain B - has CA
    auto* a4 = mol.NewAtom(8);  // Chain B
    auto* a5 = mol.NewAtom(7);  // Chain C - no CA

    a1->SetName("CA");
    a2->SetName("N");
    a3->SetName("CA");
    a4->SetName("O");
    a5->SetName("N");

    OEChem::OEResidue resA, resB, resC;
    resA.SetChainID('A');
    resA.SetResidueNumber(1);
    resB.SetChainID('B');
    resB.SetResidueNumber(1);
    resC.SetChainID('C');
    resC.SetResidueNumber(1);

    OEChem::OEAtomSetResidue(a1, resA);
    OEChem::OEAtomSetResidue(a2, resA);
    OEChem::OEAtomSetResidue(a3, resB);
    OEChem::OEAtomSetResidue(a4, resB);
    OEChem::OEAtomSetResidue(a5, resC);

    // bychain name CA should select atoms in chains A and B (both have CA)
    OESelect sel(mol, "bychain name CA");

    EXPECT_TRUE(sel(*a1));   // CA in chain A
    EXPECT_TRUE(sel(*a2));   // N in chain A
    EXPECT_TRUE(sel(*a3));   // CA in chain B
    EXPECT_TRUE(sel(*a4));   // O in chain B
    EXPECT_FALSE(sel(*a5));  // N in chain C (no CA)
}

TEST(ExpansionPredicateTest, ByResWithLogicalOp) {
    OEChem::OEGraphMol mol;
    auto* a1 = mol.NewAtom(6);  // CA in res 1
    auto* a2 = mol.NewAtom(6);  // CB in res 1
    auto* a3 = mol.NewAtom(7);  // N in res 1
    auto* a4 = mol.NewAtom(8);  // O in res 2 (no CA or CB)

    a1->SetName("CA");
    a2->SetName("CB");
    a3->SetName("N");
    a4->SetName("O");

    OEChem::OEResidue res1, res2;
    res1.SetName("ALA");
    res1.SetResidueNumber(1);
    res1.SetChainID('A');
    res2.SetName("GLY");
    res2.SetResidueNumber(2);
    res2.SetChainID('A');

    OEChem::OEAtomSetResidue(a1, res1);
    OEChem::OEAtomSetResidue(a2, res1);
    OEChem::OEAtomSetResidue(a3, res1);
    OEChem::OEAtomSetResidue(a4, res2);

    // byres (name CA or name CB) should select all atoms in res 1
    OESelect sel(mol, "byres (name CA or name CB)");

    EXPECT_TRUE(sel(*a1));   // CA in res 1
    EXPECT_TRUE(sel(*a2));   // CB in res 1
    EXPECT_TRUE(sel(*a3));   // N in res 1
    EXPECT_FALSE(sel(*a4));  // O in res 2
}

TEST(ExpansionPredicateTest, ByResNothing) {
    OEChem::OEGraphMol mol;
    auto* a1 = mol.NewAtom(6);
    auto* a2 = mol.NewAtom(7);

    a1->SetName("CA");
    a2->SetName("N");

    OEChem::OEResidue res1;
    res1.SetName("ALA");
    res1.SetResidueNumber(1);
    res1.SetChainID('A');

    OEChem::OEAtomSetResidue(a1, res1);
    OEChem::OEAtomSetResidue(a2, res1);

    // byres name NONEXISTENT - should match nothing
    OESelect sel(mol, "byres name NONEXISTENT");

    EXPECT_FALSE(sel(*a1));
    EXPECT_FALSE(sel(*a2));
}

TEST(ExpansionPredicateTest, ByChainNothing) {
    OEChem::OEGraphMol mol;
    auto* a1 = mol.NewAtom(6);
    auto* a2 = mol.NewAtom(7);

    a1->SetName("CA");
    a2->SetName("N");

    OEChem::OEResidue resA;
    resA.SetChainID('A');
    resA.SetResidueNumber(1);

    OEChem::OEAtomSetResidue(a1, resA);
    OEChem::OEAtomSetResidue(a2, resA);

    // bychain name NONEXISTENT - should match nothing
    OESelect sel(mol, "bychain name NONEXISTENT");

    EXPECT_FALSE(sel(*a1));
    EXPECT_FALSE(sel(*a2));
}

TEST(ExpansionPredicateTest, ByResToCanonical) {
    OESelection sele = OESelection::Parse("byres name CA");
    std::string canonical = sele.ToCanonical();
    EXPECT_TRUE(canonical.find("byres") != std::string::npos);
    EXPECT_TRUE(canonical.find("name CA") != std::string::npos);
}

TEST(ExpansionPredicateTest, ByChainToCanonical) {
    OESelection sele = OESelection::Parse("bychain name FE");
    std::string canonical = sele.ToCanonical();
    EXPECT_TRUE(canonical.find("bychain") != std::string::npos);
    EXPECT_TRUE(canonical.find("name FE") != std::string::npos);
}

TEST(ExpansionPredicateTest, ByResWithProtein) {
    OEChem::OEGraphMol mol;
    // Create a small protein-like structure
    auto* a1 = mol.NewAtom(6);  // Protein CA
    auto* a2 = mol.NewAtom(7);  // Protein N
    auto* a3 = mol.NewAtom(8);  // Water O

    a1->SetName("CA");
    a2->SetName("N");
    a3->SetName("O");

    OEChem::OEResidue resProtein, resWater;
    resProtein.SetName("ALA");
    resProtein.SetResidueNumber(1);
    resProtein.SetChainID('A');
    resWater.SetName("HOH");
    resWater.SetResidueNumber(100);
    resWater.SetChainID(' ');

    OEChem::OEAtomSetResidue(a1, resProtein);
    OEChem::OEAtomSetResidue(a2, resProtein);
    OEChem::OEAtomSetResidue(a3, resWater);

    // byres protein should expand protein to full residues
    OESelect sel(mol, "byres protein");

    EXPECT_TRUE(sel(*a1));   // Protein atom
    EXPECT_TRUE(sel(*a2));   // Protein atom in same residue
    EXPECT_FALSE(sel(*a3));  // Water - not protein
}

TEST(ExpansionPredicateTest, ByResCaseInsensitive) {
    OEChem::OEGraphMol mol;
    auto* a1 = mol.NewAtom(6);
    auto* a2 = mol.NewAtom(7);

    a1->SetName("CA");
    a2->SetName("N");

    OEChem::OEResidue res1;
    res1.SetName("ALA");
    res1.SetResidueNumber(1);
    res1.SetChainID('A');

    OEChem::OEAtomSetResidue(a1, res1);
    OEChem::OEAtomSetResidue(a2, res1);

    // Test case insensitivity of byres keyword
    OESelect sel_lower(mol, "byres name CA");
    OESelect sel_upper(mol, "BYRES name CA");
    OESelect sel_mixed(mol, "ByRes name CA");

    EXPECT_TRUE(sel_lower(*a1));
    EXPECT_TRUE(sel_upper(*a1));
    EXPECT_TRUE(sel_mixed(*a1));
    EXPECT_TRUE(sel_lower(*a2));
    EXPECT_TRUE(sel_upper(*a2));
    EXPECT_TRUE(sel_mixed(*a2));
}

TEST(ExpansionPredicateTest, ByChainCaseInsensitive) {
    OEChem::OEGraphMol mol;
    auto* a1 = mol.NewAtom(6);
    auto* a2 = mol.NewAtom(7);

    a1->SetName("CA");
    a2->SetName("N");

    OEChem::OEResidue resA;
    resA.SetChainID('A');
    resA.SetResidueNumber(1);

    OEChem::OEAtomSetResidue(a1, resA);
    OEChem::OEAtomSetResidue(a2, resA);

    // Test case insensitivity of bychain keyword
    OESelect sel_lower(mol, "bychain name CA");
    OESelect sel_upper(mol, "BYCHAIN name CA");
    OESelect sel_mixed(mol, "ByChain name CA");

    EXPECT_TRUE(sel_lower(*a1));
    EXPECT_TRUE(sel_upper(*a1));
    EXPECT_TRUE(sel_mixed(*a1));
    EXPECT_TRUE(sel_lower(*a2));
    EXPECT_TRUE(sel_upper(*a2));
    EXPECT_TRUE(sel_mixed(*a2));
}
