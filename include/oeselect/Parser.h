/**
 * @file Parser.h
 * @brief Selection string parser using PEGTL.
 *
 * This module provides the parsing infrastructure for PyMOL-style selection
 * strings. It uses PEGTL (Parsing Expression Grammar Template Library) to
 * parse input and construct a predicate tree.
 *
 * @section syntax Supported Selection Syntax
 *
 * @subsection keywords Property Keywords
 * - `name <pattern>` - Atom name (supports wildcards * and ?)
 * - `resn <pattern>` - Residue name (supports wildcards)
 * - `resi <number>` - Residue number (supports ranges like 1-10, comparisons like >50)
 * - `chain <id>` - Chain identifier (single character)
 * - `elem <symbol>` - Element symbol (e.g., C, Fe)
 * - `index <number>` - Atom index (supports ranges and comparisons)
 *
 * @subsection components Component Keywords
 * - `protein` - Amino acid residues
 * - `ligand` - Small molecule ligands
 * - `water` - Water molecules
 * - `solvent` - Water and common solvents
 * - `organic` - Carbon-containing non-polymer molecules
 * - `backbone` / `bb` - Protein backbone atoms (N, CA, C, O)
 * - `sidechain` / `sc` - Protein sidechain atoms
 * - `metal` / `metals` - Metal ions
 *
 * @subsection atomtype Atom Type Keywords
 * - `heavy` - Non-hydrogen atoms
 * - `hydrogen` / `h` - Hydrogen atoms
 * - `polar_hydrogen` / `polarh` - Hydrogens bonded to N, O, or S
 * - `nonpolar_hydrogen` / `apolarh` - Hydrogens bonded to C
 *
 * @subsection secstruct Secondary Structure
 * - `helix` - Alpha helix residues
 * - `sheet` - Beta sheet residues
 * - `turn` - Turn residues
 * - `loop` - Loop/coil residues
 *
 * @subsection distance Distance Operators
 * - `around <radius> <selection>` - Atoms within radius of selection
 * - `xaround <radius> <selection>` - Around, excluding reference atoms
 * - `beyond <radius> <selection>` - Atoms outside radius of selection
 *
 * @subsection expansion Expansion Operators
 * - `byres <selection>` - Expand to complete residues
 * - `bychain <selection>` - Expand to complete chains
 *
 * @subsection logical Logical Operators
 * - `and` - Intersection (higher precedence than or)
 * - `or` - Union
 * - `not` - Negation (highest precedence)
 * - `xor` - Exclusive or (lowest precedence)
 *
 * @subsection special Special Keywords
 * - `all` - All atoms
 * - `none` - No atoms
 *
 * @subsection macro Hierarchical Macro Syntax
 * - `//chain/resi/name` - Hierarchical selection (empty components are wildcards)
 *
 * @subsection multivalue Multi-Value Syntax
 * - `name CA+CB+N` - Multiple values joined with +
 */

#ifndef OESELECT_PARSER_H
#define OESELECT_PARSER_H

#include <string>

#include "oeselect/Predicate.h"

namespace OESel {

/**
 * @brief Parse a selection string into a predicate tree.
 *
 * This is the main parsing entry point. It converts a PyMOL-style selection
 * string into an executable predicate tree.
 *
 * @param sele The selection string to parse.
 * @return Shared pointer to the root predicate.
 * @throws SelectionError if the string cannot be parsed.
 *
 * @code
 * auto pred = ParseSelection("protein and chain A");
 * // pred is now a tree: And(Protein, Chain("A"))
 * @endcode
 */
Predicate::Ptr ParseSelection(const std::string& sele);

}  // namespace OESel

#endif  // OESELECT_PARSER_H
