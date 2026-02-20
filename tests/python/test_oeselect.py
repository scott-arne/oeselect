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

    # Set up residue information and atom names
    atom_idx = 0
    ala_names = ["CB", "CA", "N", "C", "O"]
    gly_names = ["N", "CA", "C", "O"]
    for atom in mol.GetAtoms():
        res = oechem.OEResidue()
        if atom_idx < 5:
            res.SetName("ALA")
            res.SetResidueNumber(1)
            res.SetChainID("A")
            res.SetInsertCode(" ")
            if atom_idx < len(ala_names):
                atom.SetName(ala_names[atom_idx])
        else:
            res.SetName("GLY")
            res.SetResidueNumber(2)
            res.SetChainID("A")
            res.SetInsertCode(" ")
            gly_idx = atom_idx - 5
            if gly_idx < len(gly_names):
                atom.SetName(gly_names[gly_idx])
        oechem.OEAtomSetResidue(atom, res)
        atom_idx += 1

    return mol


class TestOESelection:
    """Tests for OESelection parsing."""

    def test_parse_empty(self):
        """Empty selection should be valid."""
        from oeselect import parse

        sele = parse("")
        assert sele.IsEmpty()
        assert sele.ToCanonical() == "all"

    def test_parse_name(self):
        """Name selection should parse correctly."""
        from oeselect import parse

        sele = parse("name CA")
        assert not sele.IsEmpty()
        assert "name" in sele.ToCanonical().lower()

    def test_parse_logical_and(self):
        """AND expression should parse."""
        from oeselect import parse

        sele = parse("name CA and chain A")
        canonical = sele.ToCanonical()
        assert "and" in canonical.lower()

    def test_parse_logical_or(self):
        """OR expression should parse."""
        from oeselect import parse

        sele = parse("name CA or name CB")
        canonical = sele.ToCanonical()
        assert "or" in canonical.lower()

    def test_parse_invalid_raises(self):
        """Invalid selection should raise ValueError."""
        from oeselect import parse

        with pytest.raises(ValueError):
            parse("invalid_keyword xyz")

    def test_repr_and_str(self):
        """OESelection should have repr and str methods."""
        from oeselect import parse

        sele = parse("name CA")
        assert "OESelection" in repr(sele)
        assert str(sele) == sele.ToCanonical()


class TestSelect:
    """Tests for select() function."""

    def test_select_elem_carbon(self, simple_mol):
        """Select carbon atoms."""
        from oeselect import select

        indices = select(simple_mol, "elem C")
        assert len(indices) > 0

        # Verify all selected atoms are carbon
        for idx in indices:
            atom = simple_mol.GetAtom(oechem.OEHasAtomIdx(idx))
            assert atom.GetAtomicNum() == 6

    def test_select_all(self, simple_mol):
        """'all' should select all atoms."""
        from oeselect import select

        indices = select(simple_mol, "all")
        assert len(indices) == simple_mol.NumAtoms()

    def test_select_none(self, simple_mol):
        """'none' should select no atoms."""
        from oeselect import select

        indices = select(simple_mol, "none")
        assert len(indices) == 0

    def test_select_resn(self, protein_mol):
        """Select by residue name -- works with direct molecule passing."""
        from oeselect import select

        ala_indices = select(protein_mol, "resn ALA")
        assert len(ala_indices) == 5  # First 5 atoms are ALA

    def test_select_chain(self, protein_mol):
        """Select by chain -- works with direct molecule passing."""
        from oeselect import select

        chain_a_indices = select(protein_mol, "chain A")
        assert len(chain_a_indices) == protein_mol.NumAtoms()

    def test_select_resi_range(self, protein_mol):
        """Select by residue number range."""
        from oeselect import select

        indices = select(protein_mol, "resi 1-2")
        assert len(indices) == protein_mol.NumAtoms()

    def test_select_resi_single(self, protein_mol):
        """Select by single residue number."""
        from oeselect import select

        indices = select(protein_mol, "resi 1")
        assert len(indices) == 5  # First 5 atoms in residue 1

    def test_select_index_comparison(self, simple_mol):
        """Select by index comparison."""
        from oeselect import select

        # Select first 5 atoms
        indices = select(simple_mol, "index < 5")
        assert len(indices) == 5
        assert set(indices) == {0, 1, 2, 3, 4}


class TestOESelect:
    """Tests for OESelect predicate class."""

    def test_oeselect_call(self, simple_mol):
        """OESelect should evaluate atoms."""
        from oeselect import OESelect

        pred = OESelect(simple_mol, "elem C")
        # Test first atom
        atom = simple_mol.GetAtom(oechem.OEHasAtomIdx(0))
        result = pred(atom)
        assert isinstance(result, bool)

    def test_oeselect_with_getatoms(self, simple_mol):
        """OESelect should work with mol.GetAtoms()."""
        from oeselect import OESelect

        pred = OESelect(simple_mol, "elem C")
        atoms = list(simple_mol.GetAtoms(pred))
        assert len(atoms) > 0
        for atom in atoms:
            assert atom.GetAtomicNum() == 6

    def test_oeselect_with_oecount(self, simple_mol):
        """OESelect should work with oechem.OECount()."""
        from oeselect import OESelect

        pred = OESelect(simple_mol, "elem C")
        count = oechem.OECount(simple_mol, pred)
        assert count > 0

        # Verify against manual count
        manual_count = sum(1 for a in simple_mol.GetAtoms() if a.GetAtomicNum() == 6)
        assert count == manual_count

    def test_oeselect_iter(self, simple_mol):
        """OESelect should be iterable."""
        from oeselect import OESelect

        pred = OESelect(simple_mol, "elem O")
        atoms = list(pred)
        assert len(atoms) > 0
        for atom in atoms:
            assert atom.GetAtomicNum() == 8

    def test_oeselect_residue_info(self, protein_mol):
        """OESelect should preserve residue info with direct molecule passing."""
        from oeselect import OESelect

        pred = OESelect(protein_mol, "resn ALA")
        atoms = list(protein_mol.GetAtoms(pred))
        assert len(atoms) == 5

        for atom in atoms:
            res = oechem.OEAtomGetResidue(atom)
            assert res.GetName().strip() == "ALA"

    def test_oeselect_sele_property(self, simple_mol):
        """OESelect.sele should return the underlying OESelection."""
        from oeselect import OESelect

        pred = OESelect(simple_mol, "elem C")
        sele = pred.sele
        assert "elem" in sele.ToCanonical().lower()

    def test_oeselect_repr(self, simple_mol):
        """OESelect should have a repr."""
        from oeselect import OESelect

        pred = OESelect(simple_mol, "elem C")
        assert "OESelect" in repr(pred)


class TestCount:
    """Tests for count() function."""

    def test_count_all(self, simple_mol):
        """Count all atoms."""
        from oeselect import count

        num = count(simple_mol, "all")
        assert num == simple_mol.NumAtoms()

    def test_count_elem(self, simple_mol):
        """Count carbon atoms."""
        from oeselect import count

        num_carbons = count(simple_mol, "elem C")
        assert num_carbons > 0

        # Verify by manual count
        manual_count = sum(1 for a in simple_mol.GetAtoms() if a.GetAtomicNum() == 6)
        assert num_carbons == manual_count

    def test_count_resn(self, protein_mol):
        """Count by residue name -- works with direct molecule passing."""
        from oeselect import count

        num_ala = count(protein_mol, "resn ALA")
        assert num_ala == 5

        num_gly = count(protein_mol, "resn GLY")
        assert num_gly == protein_mol.NumAtoms() - 5


class TestMultiValueSyntax:
    """Tests for multi-value syntax (name CA+CB+N)."""

    def test_multi_value_name(self, protein_mol):
        """Multi-value name syntax should work with direct molecule passing."""
        from oeselect import select

        indices = select(protein_mol, "name CA+CB+N")
        assert len(indices) > 0

        for idx in indices:
            atom = protein_mol.GetAtom(oechem.OEHasAtomIdx(idx))
            assert atom.GetName().strip() in ["CA", "CB", "N"]


class TestHierarchicalMacro:
    """Tests for hierarchical macro syntax (//chain/resi/name)."""

    def test_macro_chain(self, protein_mol):
        """Select by chain using macro syntax."""
        from oeselect import select

        indices = select(protein_mol, "//A//")
        assert len(indices) == protein_mol.NumAtoms()

    def test_macro_chain_resi(self, protein_mol):
        """Select by chain and residue using macro syntax."""
        from oeselect import select

        indices = select(protein_mol, "//A/1/")
        assert len(indices) == 5  # 5 atoms in residue 1


class TestLogicalOperators:
    """Tests for logical operators."""

    def test_and_operator(self, simple_mol):
        """AND operator should work."""
        from oeselect import count

        # elem C and index < 5 should select carbons in first 5 atoms
        total_c = count(simple_mol, "elem C")
        c_first_5 = count(simple_mol, "elem C and index < 5")
        assert c_first_5 <= total_c
        assert c_first_5 <= 5

    def test_or_operator(self, simple_mol):
        """OR operator should work."""
        from oeselect import count

        c_count = count(simple_mol, "elem C")
        o_count = count(simple_mol, "elem O")
        c_or_o = count(simple_mol, "elem C or elem O")
        assert c_or_o == c_count + o_count

    def test_not_operator(self, simple_mol):
        """NOT operator should work."""
        from oeselect import count

        total = simple_mol.NumAtoms()
        c_count = count(simple_mol, "elem C")
        not_c = count(simple_mol, "not elem C")
        assert not_c == total - c_count


class TestSelector:
    """Tests for Selector struct."""

    def test_selector_from_string(self):
        """Parse selector from string."""
        from oeselect import Selector

        sel = Selector.FromString("ALA:123: :A")
        assert sel.name == "ALA"
        assert sel.residue_number == 123
        assert sel.chain == "A"
        assert sel.insert_code == " "

    def test_selector_to_string(self):
        """Convert selector to string."""
        from oeselect import Selector

        sel = Selector("ALA", 123, "A", " ")
        assert sel.ToString() == "ALA:123: :A"

    def test_selector_from_atom(self, protein_mol):
        """Extract selector from atom."""
        from oeselect import Selector

        atom = next(iter(protein_mol.GetAtoms()))
        sel = Selector.FromAtom(atom)
        assert sel.name.strip() == "ALA"
        assert sel.residue_number == 1
        assert sel.chain == "A"

    def test_selector_roundtrip(self):
        """FromString -> ToString should roundtrip."""
        from oeselect import Selector

        original = "ALA:123: :A"
        sel = Selector.FromString(original)
        assert sel.ToString() == original

    def test_selector_comparison(self):
        """Selectors should compare by (chain, residue_number, insert_code)."""
        from oeselect import Selector

        sel_a1 = Selector("ALA", 1, "A")
        sel_a2 = Selector("GLY", 2, "A")
        sel_b1 = Selector("ALA", 1, "B")

        assert sel_a1 < sel_a2
        assert sel_a2 < sel_b1
        assert sel_a1 == Selector("ALA", 1, "A")
        assert sel_a1 != sel_a2

    def test_selector_equality(self):
        """Selector equality includes name."""
        from oeselect import Selector

        sel1 = Selector("ALA", 1, "A")
        sel2 = Selector("GLY", 1, "A")
        assert sel1 != sel2  # Different name

    def test_selector_hash(self):
        """Selectors should be hashable for use in sets."""
        from oeselect import Selector

        sel1 = Selector.FromString("ALA:1: :A")
        sel2 = Selector.FromString("ALA:1: :A")
        assert hash(sel1) == hash(sel2)
        assert {sel1, sel2} == {sel1}

    def test_selector_invalid_string(self):
        """Invalid selector string should raise."""
        from oeselect import Selector

        with pytest.raises(ValueError):
            Selector.FromString("invalid")


class TestOEResidueSelector:
    """Tests for OEResidueSelector predicate."""

    def test_residue_selector_single(self, protein_mol):
        """Select atoms by single residue selector."""
        from oeselect import OEResidueSelector

        sel = OEResidueSelector("ALA:1: :A")
        atoms = list(protein_mol.GetAtoms(sel))
        assert len(atoms) == 5

    def test_residue_selector_multiple(self, protein_mol):
        """Select atoms by multiple residue selectors."""
        from oeselect import OEResidueSelector

        sel = OEResidueSelector("ALA:1: :A,GLY:2: :A")
        atoms = list(protein_mol.GetAtoms(sel))
        assert len(atoms) == protein_mol.NumAtoms()


class TestCustomPredicates:
    """Tests for custom predicate classes."""

    def test_has_residue_name_case_insensitive(self, protein_mol):
        """OEHasResidueName should be case-insensitive by default."""
        from oeselect import OEHasResidueName

        pred = OEHasResidueName("ala")  # Lowercase
        atoms = list(protein_mol.GetAtoms(pred))
        assert len(atoms) == 5

    def test_has_residue_name_case_sensitive(self, protein_mol):
        """OEHasResidueName should respect case when case_sensitive=True."""
        from oeselect import OEHasResidueName

        pred_upper = OEHasResidueName("ALA", case_sensitive=True)
        pred_lower = OEHasResidueName("ala", case_sensitive=True)

        upper_atoms = list(protein_mol.GetAtoms(pred_upper))
        lower_atoms = list(protein_mol.GetAtoms(pred_lower))

        assert len(upper_atoms) == 5
        assert len(lower_atoms) == 0

    def test_has_atom_name_case_insensitive(self, protein_mol):
        """OEHasAtomNameAdvanced should be case-insensitive by default."""
        from oeselect import OEHasAtomNameAdvanced

        pred = OEHasAtomNameAdvanced("ca")  # Lowercase
        atoms = list(protein_mol.GetAtoms(pred))
        assert len(atoms) > 0
        for atom in atoms:
            assert atom.GetName().strip().upper() == "CA"

    def test_has_atom_name_case_sensitive(self, protein_mol):
        """OEHasAtomNameAdvanced should respect case when case_sensitive=True."""
        from oeselect import OEHasAtomNameAdvanced

        pred = OEHasAtomNameAdvanced("CA", case_sensitive=True)
        atoms = list(protein_mol.GetAtoms(pred))
        assert len(atoms) > 0


class TestSelectorUtilities:
    """Tests for selector utility functions."""

    def test_str_selector_set(self, protein_mol):
        """str_selector_set should return selector strings for matching atoms."""
        from oeselect import str_selector_set

        selectors = str_selector_set(protein_mol, "all")
        assert len(selectors) == 2  # ALA:1 and GLY:2
        assert any("ALA" in s for s in selectors)
        assert any("GLY" in s for s in selectors)

    def test_selector_set_parse(self):
        """selector_set should parse multi-selector strings."""
        from oeselect import selector_set

        sels = selector_set("ALA:1: :A,GLY:2: :A")
        assert len(sels) == 2

    def test_mol_to_selector_set(self, protein_mol):
        """mol_to_selector_set should extract all unique residue selectors."""
        from oeselect import mol_to_selector_set

        selectors = mol_to_selector_set(protein_mol)
        assert len(selectors) == 2

    def test_get_selector_string(self, protein_mol):
        """get_selector_string should return selector for an atom."""
        from oeselect import get_selector_string

        atom = next(iter(protein_mol.GetAtoms()))
        sel_str = get_selector_string(atom)
        assert "ALA" in sel_str
        assert "1" in sel_str
        assert "A" in sel_str


class TestPredicateType:
    """Tests for PredicateType enum."""

    def test_contains_predicate(self):
        """ContainsPredicate should work."""
        from oeselect import parse, PredicateType

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
