"""
Unit tests for OESelect Python bindings

These tests require the OpenEye Toolkits Python package to be installed.
"""

import pytest


@pytest.fixture
def simple_mol():
    """Create a simple molecule (aspirin) for testing."""
    from openeye import oechem

    mol = oechem.OEGraphMol()
    oechem.OESmilesToMol(mol, "CC(=O)OC1=CC=CC=C1C(=O)O")  # Aspirin
    oechem.OEAssignAromaticFlags(mol)
    return mol


@pytest.fixture
def protein_mol():
    """Create a simple peptide-like molecule with residue info."""
    from openeye import oechem

    mol = oechem.OEGraphMol()
    oechem.OESmilesToMol(mol, "CC(N)C(=O)NCC(=O)O")  # Simplified dipeptide

    # Set up residue information
    atom_idx = 0
    for atom in mol.GetAtoms():
        res = oechem.OEResidue()
        if atom_idx < 5:
            res.SetName("ALA")
            res.SetResidueNumber(1)
            res.SetChainID("A")
        else:
            res.SetName("GLY")
            res.SetResidueNumber(2)
            res.SetChainID("A")
        oechem.OEAtomSetResidue(atom, res)
        atom_idx += 1

    return mol


class TestOESelection:
    """Tests for OESelection parsing."""

    def test_parse_empty(self):
        """Empty selection should be valid."""
        from oeselect_lib import parse

        sele = parse("")
        assert sele.IsEmpty()
        assert sele.ToCanonical() == "all"

    def test_parse_name(self):
        """Name selection should parse correctly."""
        from oeselect_lib import parse

        sele = parse("name CA")
        assert not sele.IsEmpty()
        assert "name" in sele.ToCanonical().lower()

    def test_parse_logical_and(self):
        """AND expression should parse."""
        from oeselect_lib import parse

        sele = parse("name CA and chain A")
        canonical = sele.ToCanonical()
        assert "and" in canonical.lower()

    def test_parse_logical_or(self):
        """OR expression should parse."""
        from oeselect_lib import parse

        sele = parse("name CA or name CB")
        canonical = sele.ToCanonical()
        assert "or" in canonical.lower()

    def test_parse_invalid_raises(self):
        """Invalid selection should raise ValueError."""
        from oeselect_lib import parse

        with pytest.raises(ValueError):
            parse("invalid_keyword xyz")

    def test_repr_and_str(self):
        """OESelection should have repr and str methods."""
        from oeselect_lib import parse

        sele = parse("name CA")
        assert "OESelection" in repr(sele)
        assert str(sele) == sele.ToCanonical()


class TestSelect:
    """Tests for select() function."""

    def test_select_elem_carbon(self, simple_mol):
        """Select carbon atoms."""
        from oeselect_lib import select

        indices = select("elem C", simple_mol)
        assert len(indices) > 0

        # Verify all selected atoms are carbon
        for idx in indices:
            atom = simple_mol.GetAtom(oechem.OEHasAtomIdx(idx))
            assert atom.GetAtomicNum() == 6

    def test_select_all(self, simple_mol):
        """'all' should select all atoms."""
        from oeselect_lib import select

        indices = select("all", simple_mol)
        assert len(indices) == simple_mol.NumAtoms()

    def test_select_none(self, simple_mol):
        """'none' should select no atoms."""
        from oeselect_lib import select

        indices = select("none", simple_mol)
        assert len(indices) == 0

    @pytest.mark.skip(reason="SMILES conversion loses residue info; need direct OEMol access")
    def test_select_resn(self, protein_mol):
        """Select by residue name."""
        from oeselect_lib import select

        ala_indices = select("resn ALA", protein_mol)
        assert len(ala_indices) == 5  # First 5 atoms are ALA

    @pytest.mark.skip(reason="SMILES conversion loses chain info; need direct OEMol access")
    def test_select_chain(self, protein_mol):
        """Select by chain."""
        from oeselect_lib import select

        chain_a_indices = select("chain A", protein_mol)
        assert len(chain_a_indices) == protein_mol.NumAtoms()  # All atoms in chain A

    @pytest.mark.skip(reason="SMILES conversion loses residue info; need direct OEMol access")
    def test_select_resi_range(self, protein_mol):
        """Select by residue number range."""
        from oeselect_lib import select

        indices = select("resi 1-2", protein_mol)
        assert len(indices) == protein_mol.NumAtoms()

    def test_select_index_comparison(self, simple_mol):
        """Select by index comparison."""
        from oeselect_lib import select

        # Select first 5 atoms
        indices = select("index < 5", simple_mol)
        assert len(indices) == 5
        assert set(indices) == {0, 1, 2, 3, 4}


class TestCount:
    """Tests for count() function."""

    def test_count_all(self, simple_mol):
        """Count all atoms."""
        from oeselect_lib import count

        num = count("all", simple_mol)
        assert num == simple_mol.NumAtoms()

    def test_count_elem(self, simple_mol):
        """Count carbon atoms."""
        from oeselect_lib import count

        num_carbons = count("elem C", simple_mol)
        assert num_carbons > 0

        # Verify by manual count
        manual_count = sum(1 for a in simple_mol.GetAtoms() if a.GetAtomicNum() == 6)
        assert num_carbons == manual_count


class TestMultiValueSyntax:
    """Tests for multi-value syntax (name CA+CB+N)."""

    @pytest.mark.skip(reason="SMILES conversion loses atom names; need direct OEMol access")
    def test_multi_value_name(self, protein_mol):
        """Multi-value name syntax should work."""
        from oeselect_lib import select

        # Set up atom names
        names = ["CA", "CB", "N", "C", "O", "CA", "N", "C", "O"]
        for atom, name in zip(protein_mol.GetAtoms(), names):
            atom.SetName(name)

        # Select multiple names
        indices = select("name CA+CB+N", protein_mol)
        assert len(indices) > 0

        # Verify selected atoms have correct names
        for idx in indices:
            atom = protein_mol.GetAtom(oechem.OEHasAtomIdx(idx))
            assert atom.GetName() in ["CA", "CB", "N"]


class TestHierarchicalMacro:
    """Tests for hierarchical macro syntax (//chain/resi/name)."""

    @pytest.mark.skip(reason="SMILES conversion loses chain info; need direct OEMol access")
    def test_macro_chain(self, protein_mol):
        """Select by chain using macro syntax."""
        from oeselect_lib import select

        indices = select("//A//", protein_mol)
        assert len(indices) == protein_mol.NumAtoms()

    @pytest.mark.skip(reason="SMILES conversion loses residue info; need direct OEMol access")
    def test_macro_chain_resi(self, protein_mol):
        """Select by chain and residue using macro syntax."""
        from oeselect_lib import select

        indices = select("//A/1/", protein_mol)
        assert len(indices) == 5  # 5 atoms in residue 1


class TestLogicalOperators:
    """Tests for logical operators."""

    def test_and_operator(self, simple_mol):
        """AND operator should work."""
        from oeselect_lib import count

        # elem C and index < 5 should select carbons in first 5 atoms
        total_c = count("elem C", simple_mol)
        c_first_5 = count("elem C and index < 5", simple_mol)
        assert c_first_5 <= total_c
        assert c_first_5 <= 5

    def test_or_operator(self, simple_mol):
        """OR operator should work."""
        from oeselect_lib import count

        c_count = count("elem C", simple_mol)
        o_count = count("elem O", simple_mol)
        c_or_o = count("elem C or elem O", simple_mol)
        assert c_or_o == c_count + o_count

    def test_not_operator(self, simple_mol):
        """NOT operator should work."""
        from oeselect_lib import count

        total = simple_mol.NumAtoms()
        c_count = count("elem C", simple_mol)
        not_c = count("not elem C", simple_mol)
        assert not_c == total - c_count


class TestPredicateType:
    """Tests for PredicateType enum."""

    def test_contains_predicate(self):
        """ContainsPredicate should work."""
        from oeselect_lib import parse, PredicateType

        sele = parse("name CA")
        assert sele.ContainsPredicate(PredicateType.Name)
        assert not sele.ContainsPredicate(PredicateType.Protein)

        sele2 = parse("protein and chain A")
        assert sele2.ContainsPredicate(PredicateType.Protein)
        assert sele2.ContainsPredicate(PredicateType.Chain)
        assert sele2.ContainsPredicate(PredicateType.And)


# Import oechem at module level for fixtures
try:
    from openeye import oechem
except ImportError:
    pytest.skip("OpenEye Toolkits not available", allow_module_level=True)
